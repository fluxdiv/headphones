#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>



// #include <cstdint>
// #include <cstdio>
#include <iostream>
#include "listener.h"
#include <sys/types.h>
#include <netinet/in.h>
// #include <stdio.h>
// #include <stdlib.h>

int test();
int foo();

int main() {
    // std::cout << "Hello, World!" << std::endl;
    int r = test();
    printf("r value: %d \n", r);
    // 4096
    // int page_size = getpagesize();
    // printf("Page size: %d \n", page_size);
    // int r = foo();
    // printf("r: %d \n", r);
    return 0;
}

int foo() {
    int *xs = static_cast<int*>(mmap(
        NULL,
        10 * sizeof(int),
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        0,
        0
    ));

    if (xs == (int*) -1) {
        perror("error calling mmap");
        return -1;
    }

    xs[0] = 0;
    xs[1] = 1;

    for (int i = 2; i < 10; ++i) {
        xs[i] = xs[i - 1] + xs[i - 2];
    }

    int err = munmap(xs, 10 * sizeof(int));
    if (err < 0) {
        perror("error in unmap");
        return -1;
    };

    return 0;
}

// https://www.kernel.org/doc/Documentation/networking/packet_mmap.txt
//
// [setup]     socket() -------> creation of the capture socket
//             setsockopt() ---> allocation of the circular buffer (ring)
//                               option: PACKET_RX_RING
//             mmap() ---------> mapping of the allocated buffer to the
//                               user process
//
// [capture]   poll() ---------> to wait for incoming packets
//
// [shutdown]  close() --------> destruction of the capture socket and
//                               deallocation of all associated 
//                               resources.

volatile int fd;

int test() {
    // [setup] ===========================================================
    // create socket for capturing packets
    fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        perror("error creating socket");
        return -1;
    }

    // printf("socket fd: %d \n", fd);
    // allocation of ring buffer with option PACKET_RX_RING
    // struct tpacket_req {
    //     unsigned int	tp_block_size;	/* Minimal size of contiguous block */
    //     unsigned int	tp_block_nr;	/* Number of blocks */
    //     unsigned int	tp_frame_size;	/* Size of frame */
    //     unsigned int	tp_frame_nr;	/* Total number of frames */
    // };

    //4096
    int page_size = getpagesize();

    struct tpacket_req req;
    // memset(&req, 0, sizeof(req));
    unsigned int blocksiz = 1 << 22;
    unsigned int framesiz = 1 << 11;
    unsigned int blocknum = 64;
    req.tp_block_size = blocksiz;
    req.tp_frame_size = framesiz;
    req.tp_block_nr = blocknum;
    req.tp_frame_nr = (blocksiz * blocknum) / framesiz;

    // req.tp_block_size = page_size;
    // req.tp_block_nr = 4;
    // req.tp_frame_size = page_size / 2;
    // req.tp_frame_nr = 8;

    // TPACKET_HDRLEN;
    // TPACKET_ALIGNMENT;

    // Size of memory to mmap in kernel
    // aka total size of all blocks
    uint32_t size_of_mmap = req.tp_block_size * req.tp_block_nr;

    int set_sock_opt_res = setsockopt(
        fd,             //int fd,
        // SOL_SOCKET,  //int level,
        SOL_PACKET,
        PACKET_RX_RING, //int optname,
        &req,           //const void *optval,
        sizeof(req)     //socklen_t optlen
    );
    if (set_sock_opt_res < 0) {
        perror("Error setting sock opt for option PACKET_RX_RING");
        return -1;
    };
    printf("sock opt res: %d \n", set_sock_opt_res);

    // HERE
    // I think the problem is size
    // mmap the rx ring buffer memory
    // void *mmap_ptr = mmap(
    //     0, // void *addr,
    //     size_of_mmap, //size_t len,
    //     PROT_READ,
    //     MAP_SHARED, 
    //     fd,
    //     0
    // );
    int* mmap_ptr = static_cast<int*>(mmap(
        NULL, // void *addr,
        // size_of_mmap, //size_t len,
        // 2 * page_size,
        size_of_mmap,
        // req.tp_block_size * req.tp_block_nr,
        PROT_READ,
        MAP_SHARED, 
        fd,
        0
    ));
    if (mmap_ptr == MAP_FAILED) {
        // error calling mmap: Invalid argument
        perror("Error calling mmap");
        return -1;
    };

    // [capture] ===========================================================
    // poll to wait for incoming packets

    // [shutdown] ===========================================================
    // destruction of capture socket & deallocation of resources
    close(fd);

    return 0;
}
