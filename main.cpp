#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <poll.h>
#include <unistd.h>
#include <signal.h>
#include <inttypes.h>
#include <vector>

#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>

#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/user.h>

#include <linux/ip.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#include <sys/ioctl.h>

#include <fmt/core.h>

#include "eth_lookup.hpp"
#include "display.hpp"
#include "prom.hpp"

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif

//---------------------------------------------------------
// Constants for size of mmap
static const unsigned long frame_size = PAGE_SIZE << 10;
static const unsigned long block_size = PAGE_SIZE << 11;
// static const unsigned long frame_size = 1 << 11;
// static const unsigned long block_size = 1 << 22;
static const unsigned int blocks = 64;
//---------------------------------------------------------
// State
volatile int socket_fd;
static unsigned long packets_total = 0, bytes_total = 0;
static sig_atomic_t sigint = 0;

//---------------------------------------------------------
struct block_desc {
	uint32_t version;
	uint32_t offset_to_priv;
	struct tpacket_hdr_v1 h1;
};

struct ring {
    // array of iovec
    struct iovec *rd;
    // pointer to memory map of packet backing
    uint8_t *map;
    // requirements for RX ring buf allocated in setsockopt(PACKET_RX_RING)
    struct tpacket_req3 req;
};

// display info for each packet
struct packet_disp {
    unsigned int rxhash;
    unsigned short proto;
    char src[NI_MAXHOST];
    char dest[NI_MAXHOST];
    unsigned int secs;
};

//---------------------------------------------------------
static int init_socket(ring *ring, char *netdev);
static void teardown_socket(struct ring *ring, int fd);
static void extract_packet(tpacket3_hdr *ppd, int pnum, std::vector<packet_disp> *packets);
static void walk_block(block_desc *pbd, std::vector<packet_disp> *packets);
static void sighandler(const int signal) {sigint = 1;}
//---------------------------------------------------------

// When the application has finished processing
// a packet, it transfers ownership of the slot 
// back to the socket by setting tp_status equal to TP_STATUS_KERNEL.
static void flush_block(block_desc *pbd) {
	pbd->h1.block_status = TP_STATUS_KERNEL;
}

static void walk_block(block_desc *pbd, std::vector<packet_disp> *packets) {
    int block_status = pbd->h1.block_status;
    int num_packets = pbd->h1.num_pkts;

    unsigned long bytes = 0;
    tpacket3_hdr *ppd;

	ppd = (struct tpacket3_hdr*) ((uint8_t*) pbd + pbd->h1.offset_to_first_pkt);

    for (int i = 0; i < num_packets; ++i) {
        // flag not value
        if ((ppd->tp_status & TP_STATUS_USER) != 0) {
            bytes += ppd->tp_snaplen;
            extract_packet(ppd, i, packets);
            packets_total++;
        }
		ppd = (struct tpacket3_hdr *) ((uint8_t *) ppd + ppd->tp_next_offset);
    }
    // packets_total += num_packets;
    bytes_total += bytes;
}
// struct tpacket3_hdr {
// 	__u32		tp_next_offset;
// 	__u32		tp_sec;
// 	__u32		tp_nsec;
// 	__u32		tp_snaplen;
// 	__u32		tp_len;
// 	__u32		tp_status;
// 	__u16		tp_mac;
// 	__u16		tp_net;
// 	/* pkt_hdr variants */
// 	union {
// 		struct tpacket_hdr_variant1 hv1;
// 	};
// 	__u8		tp_padding[8];
// };

static void extract_packet(tpacket3_hdr *ppd, int pnum, std::vector<packet_disp> *packets) {
    packet_disp p = {0};
    p.rxhash = ppd->hv1.tp_rxhash;

	struct ethhdr *eth = (struct ethhdr *) ((uint8_t *) ppd + ppd->tp_mac);
	struct iphdr *ip = (struct iphdr *) ((uint8_t *) eth + ETH_HLEN);

    // 1544 is ETH_P_ARP
    // printf("1544: 0x%x\n", ntohs(1544));
    // printf("1544: %d\n", ntohs(1544));
    // // 56710 is ETH_P_IPV6
    // printf("56710: 0x%x\n", ntohs(56710));
    // printf("56710: %d\n", ntohs(56710));
    // printf("Eth Protocol: %s\n", eth_p_human(ntohs(eth->h_proto)));
    p.proto = eth->h_proto;
    p.secs = ppd->tp_sec;
	if (eth->h_proto == htons(ETH_P_IP)) {
		struct sockaddr_in ss, sd;
        char sbuff[NI_MAXHOST], dbuff[NI_MAXHOST];

		memset(&ss, 0, sizeof(ss));
		ss.sin_family = PF_INET;
		ss.sin_addr.s_addr = ip->saddr;
		getnameinfo((struct sockaddr *) &ss, sizeof(ss),
			    sbuff, sizeof(sbuff), NULL, 0, NI_NUMERICHOST);

		memset(&sd, 0, sizeof(sd));
		sd.sin_family = PF_INET;
		sd.sin_addr.s_addr = ip->daddr;
		getnameinfo((struct sockaddr *) &sd, sizeof(sd),
			    dbuff, sizeof(dbuff), NULL, 0, NI_NUMERICHOST);

        strncpy(p.src, sbuff, NI_MAXHOST);
        strncpy(p.dest, dbuff, NI_MAXHOST);
	}

    packets->insert(packets->begin(), p);
}

/// - Returns -1 on error, 0 otherwise
/// --------------------------
/// - Creates socket
/// ---- Sets global `socket_fd` to fd
/// - Sets socket version to tpacket_v3
/// --------------------------
/// - Sets param `ring->req`, requirements used for ringbuf
/// - Sets socket opt to PACKET_RX_RING
/// ---- (Doing so allocates ringbuf for packets)
/// --------------------------
/// - Create mmap (backed by ringbuf)
/// ---- Sets `ring->map` to ptr to this mmap
/// --------------------------
/// - Sets `ring->rd` to ptr to array of iovec by:
/// ---- Allocates 1 iovec for each block,
/// -------- iovec.iov_base is ptr to start of block
/// -------- iovec.iov_len is size of block
/// --------------------------
/// - Binds socket which "turns socket on" in this context
/// ---- Uses param `netdev` (provided in CLI args) for 'eth0' or 'lo'
static int init_socket(ring *ring, char *netdev) {
    int err, pack_v = TPACKET_V3;

    socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (socket_fd < 0) {
        perror("creating socket");
        return -1;
    }

    err = setsockopt(socket_fd, SOL_PACKET, PACKET_VERSION, &pack_v, sizeof(pack_v));
    if (err < 0) {
        perror("setsockopt tpacket_v3");
        return -1;
    }

    memset(&ring->req, 0, sizeof(ring->req));
	ring->req.tp_block_size = block_size;
	ring->req.tp_frame_size = frame_size;
	ring->req.tp_block_nr = blocks;
	ring->req.tp_frame_nr = (block_size * blocks) / frame_size;
	ring->req.tp_retire_blk_tov = 60;
	ring->req.tp_feature_req_word = TP_FT_REQ_FILL_RXHASH;

    // setsockopt(PACKET_RX_RING) allocates the ringbuf
    err = setsockopt(
        socket_fd,
        SOL_PACKET,
        PACKET_RX_RING,
        &ring->req,
        sizeof(ring->req)
    );
    if (err < 0) {
        perror("setsockopt PACKET_RX_RING:");
        return -1;
    }
    
    // mmap(socket_fd) creates mmap backed by the ringbuf
    ring->map = static_cast<uint8_t*>(mmap(
        NULL,
        ring->req.tp_block_size * ring->req.tp_block_nr,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_LOCKED,
        socket_fd,
        0
    ));
    if (ring->map == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

    // ---------------------------------------
    // Used to find start of blocks
    // allocate 1 iovec for each block
	ring->rd = static_cast<iovec*>(malloc(ring->req.tp_block_nr * sizeof(*ring->rd)));
    // for 0..number of blocks (for each iovec)
	for (unsigned int i = 0; i < ring->req.tp_block_nr; i++) {
        // iovec.iov_base = pointer to start of block
		ring->rd[i].iov_base = ring->map + (i * ring->req.tp_block_size);
        // iovec.iov_len = size of block
		ring->rd[i].iov_len = ring->req.tp_block_size;
	}
    // ---------------------------------------
    // ---------------------------------------
    // "bind(2) can optionally be called with a 
    // nonzero sll_protocol to start receiving packets for the protocols specified."
    // So binding here "turns the socket on" in this (AF_PACKET socket) context
    // For desc. of sockaddr_ll: https://man7.org/linux/man-pages/man7/packet.7.html
    sockaddr_ll ll;
    memset(&ll, 0, sizeof(ll));
    ll.sll_family = AF_PACKET;
    ll.sll_protocol = htons(ETH_P_ALL);
    // if_nameindex() shows that `lo` & `eth0` are only options here on my arch
    // if not valid returns 0, which matches any interface
    ll.sll_ifindex = if_nametoindex(netdev);

    // sll_hatype is an ARP type as defined in the
    // <linux/if_arp.h> include file.
    ll.sll_hatype = 0;
    // sll_pkttype
    //        contains the packet type.  Valid types are PACKET_HOST for
    //        a packet addressed to the local host, PACKET_BROADCAST for
    //        a physical-layer broadcast packet, PACKET_MULTICAST for a
    //        packet sent to a physical-layer multicast address,
    //        PACKET_OTHERHOST for a packet to some other host that has
    //        been caught by a device driver in promiscuous mode, and
    //        PACKET_OUTGOING for a packet originating from the local
    //        host that is looped back to a packet socket.  These types
    //        make sense only for receiving.
    ll.sll_pkttype = 0;
    ll.sll_halen = 0;
    // "sll_pkttype" & "sll_hatype" gets set on received packets, use in display?

    err = bind(socket_fd, (struct sockaddr*) &ll, sizeof(ll));
    if (err < 0) {
        perror("bind:");
        return -1;
    }

    return 0;
}

// Unmap socket mmap & free resources
static void teardown_socket(struct ring *ring, int fd) {
	munmap(ring->map, ring->req.tp_block_size * ring->req.tp_block_nr);
	free(ring->rd);
	close(fd);
}

int main(int argc, char **argp) {
    if (argc != 2) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "`%s eth0` - Listen for packets on eth0 interface\n", argp[0]);
        fprintf(stderr, "`%s pon`  - Turn promiscuous for eth0 on\n", argp[0]);
        fprintf(stderr, "`%s poff` - Turn promiscuous for eth0 off\n", argp[0]);
        fprintf(stderr, "`%s pget` - Check promiscuous for eth0\n", argp[0]);
        return EXIT_FAILURE;
    }

    if (std::string_view(argp[1]) == "pon") {
        int r = toggle_prom_mode_on();
        exit(r);
    } else if (std::string_view(argp[1]) == "poff") {
        int r = toggle_prom_mode_off();
        exit(r);
    } else if (std::string_view(argp[1]) == "pget") {
        int r = get_prom_mode();
        exit(r);
    }

    // [setup] ===========================================================
    int err = 0, block_idx = 0;

    ring ring;
    memset(&ring, 0, sizeof(ring));

    pollfd pfd;
    memset(&pfd, 0, sizeof(pfd));

    struct block_desc *pbd;

    // size this according to rows unless in dump mode,
    // pain with resizing but prevents huge memory on long running
    std::vector<packet_disp> packets;
    packets.reserve(1000);

    // -----------------------
    struct sigaction action;
    action.sa_handler = sighandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    int ret = sigaction(SIGINT, &action, NULL);
    if (ret == -1) {exit(1);}
    // -----------------------

    err = init_socket(&ring, argp[1]);
    if (err < 0) {
        exit(1);
    }

    pfd.fd = socket_fd;
    pfd.events = POLLIN | POLLERR;
    pfd.revents = 0;

    // ---- Display
    // alternate screen
    // std::cout << "\x1b[?1049h" << std::flush;
    // winsize w = {0};
    // ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int rows,cols;
    initscr();
    getmaxyx(stdscr, rows, cols);
    print_stats(cols, packets_total, bytes_total);
    print_table_header(cols);
    refresh();
    // [running] ===========================================================
    // poll to wait for incoming packets
    while (likely(!sigint)) {
        pbd = (block_desc*) ring.rd[block_idx].iov_base;
        // block until event occurs on socket
		if ((pbd->h1.block_status & TP_STATUS_USER) == 0) {
			poll(&pfd, 1, -1);
			continue;
		}

		walk_block(pbd, &packets);
		flush_block(pbd);
		block_idx = (block_idx + 1) % blocks;
        print_stats(cols, packets_total, bytes_total);
        print_table_header(cols);

        int n = packets.size();
        // print packets
        for (int i = 4; i < rows; i++) {
            if ((i - 4) < n) {
                packet_disp p = packets.at(i - 4);
                move(i, hash_start);
                // clear previous packet data from line
                clrtoeol();
                printw("0x%x", p.rxhash);
                move(i, proto_start);
                printw("%s", eth_p_human(ntohs(p.proto)));
                move(i, src_start);
                printw("%s", p.src);
                move(i, dest_start);
                printw("%s", p.dest);
                move(i, timestamp_start);
                printw("%u", p.secs);
            } else {
                break;
            }
        }
        refresh();
    }

    // [shutdown] ===========================================================
    // exit alt screen
    // std::cout << "\x1b[?1049l" << std::flush;
    endwin();

    // Print final exit stats (packet statistics), then
    // destruction of capture socket & deallocation of resources
	tpacket_stats_v3 stats;
    memset(&stats, 0, sizeof(stats));
    socklen_t len = sizeof(stats);
    err = getsockopt(socket_fd, SOL_PACKET, PACKET_STATISTICS, &stats, &len);
    if (err < 0) {
        perror("getsockopt final stats:");
        exit(1);
    }
    fflush(stdout);
    printf("===============================================");
    printf("\nReceived %u packets, %lu bytes, %u dropped, freeze_q_cnt: %u\n",
	       stats.tp_packets, bytes_total, stats.tp_drops,
	       stats.tp_freeze_q_cnt);
    printf("===============================================");

    teardown_socket(&ring, socket_fd);
    return 0;
}

