#ifndef RAWRECV
#define RAWRECV
#include "protocol.h"
#include <stdlib.h>
#include <linux/if_packet.h>

int readRawPacket(int socket_fd, ethernet_header* ethernet_header, mip_header* mip_header, char* payload, int* interface);
int sendRawPacket(int socket_fd, struct sockaddr_ll* sock_addr, mip_header* mip_header, char* buffer, int len, u_int8_t dst_addr[ETH_ALEN]);
#endif
