#pragma once
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <net/if.h>
#include <string.h>

int toggle_prom_mode_on();
int toggle_prom_mode_off();
// prints current promiscuous mode && flags
int get_prom_mode();
