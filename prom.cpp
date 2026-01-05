#include "prom.hpp"

int get_prom_mode() {
    // get prev
    int s = socket(AF_PACKET, SOCK_RAW, 0);
    if (s < 0) {
        perror("socket");
        return -1;
    }
    
    // print prev
    struct ifreq req;
    memset(&req, 0, sizeof(req));
    strcpy(req.ifr_ifrn.ifrn_name, "eth0"); 
    int flagres = ioctl(s, SIOCGIFFLAGS, &req);
    if (flagres == -1) {
        perror("Get SIOCGIFFLAGS:");
        close(s);
        return -1;
    }
    if ((req.ifr_ifru.ifru_flags & IFF_PROMISC) != 0) {
        printf("Promiscuous mode is currently ON\n");
    } else {
        printf("Promiscuous mode is currently OFF\n");
    }
    printf("Current flags: %d\n", req.ifr_ifru.ifru_flags);
    return 0;
}
// but muh dry bro!

int toggle_prom_mode_on() {
    // get prev
    int s = socket(AF_PACKET, SOCK_RAW, 0);
    // int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return -1;
    }
    
    // print prev
    struct ifreq req;
    memset(&req, 0, sizeof(req));
    strcpy(req.ifr_ifrn.ifrn_name, "eth0"); 
    int flagres = ioctl(s, SIOCGIFFLAGS, &req);
    if (flagres == -1) {
        perror("Get SIOCGIFFLAGS:");
        close(s);
        return -1;
    }
    if ((req.ifr_ifru.ifru_flags & IFF_PROMISC) != 0) {
        printf("Promiscuous mode already on. Current flags: %d\n", req.ifr_ifru.ifru_flags);
        return 0;
    }
    printf("Flags before toggling promiscuous mode on: %d\n", req.ifr_ifru.ifru_flags);

    // toggle it on
    req.ifr_ifru.ifru_flags |= IFF_PROMISC;
    int sflagres = ioctl(s, SIOCSIFFLAGS, &req);
    if (sflagres == -1) {
        perror("Set SIOCSIFFLAGS");
        close(s);
        return -1;
    }

    // print new
    struct ifreq req2;
    memset(&req2, 0, sizeof(req2));
    strcpy(req2.ifr_ifrn.ifrn_name, "eth0"); 
    int flagres2 = ioctl(s, SIOCGIFFLAGS, &req2);
    if (flagres2 == -1) {
        perror("Get SIOCGIFFLAGS 2:");
        close(s);
        return -1;
    }
    printf("Flags after toggling promiscuous mode on: %d\n", req2.ifr_ifru.ifru_flags);

    return 0;
}

int toggle_prom_mode_off() {
    // get prev
    int s = socket(AF_PACKET, SOCK_RAW, 0);
    if (s < 0) {
        perror("socket");
        return -1;
    }
    
    // print prev
    struct ifreq req;
    memset(&req, 0, sizeof(req));
    strcpy(req.ifr_ifrn.ifrn_name, "eth0"); 
    int flagres = ioctl(s, SIOCGIFFLAGS, &req);
    if (flagres == -1) {
        perror("Get SIOCGIFFLAGS:");
        close(s);
        return -1;
    }
    if ((req.ifr_ifru.ifru_flags & IFF_PROMISC) == 0) {
        printf("Promiscuous mode already off. Current flags: %d\n", req.ifr_ifru.ifru_flags);
        return 0;
    }
    printf("Flags before toggling promiscuous mode off: %d\n", req.ifr_ifru.ifru_flags);

    // toggle it off
    req.ifr_ifru.ifru_flags &= (~IFF_PROMISC);
    int sflagres = ioctl(s, SIOCSIFFLAGS, &req);
    if (sflagres == -1) {
        perror("Set SIOCSIFFLAGS");
        close(s);
        return -1;
    }

    // print new
    struct ifreq req2;
    memset(&req2, 0, sizeof(req2));
    strcpy(req2.ifr_ifrn.ifrn_name, "eth0"); 
    int flagres2 = ioctl(s, SIOCGIFFLAGS, &req2);
    if (flagres2 == -1) {
        perror("Get SIOCGIFFLAGS 2:");
        close(s);
        return -1;
    }
    printf("Flags after toggling promiscuous mode off: %d\n", req2.ifr_ifru.ifru_flags);

    return 0;
}
