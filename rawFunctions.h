#ifndef RAWFUNC
#define RAWFUNC
#include "protocol.h" //Mip_header, ethernet_header.
#include <linux/if_packet.h> //ETH_ALEN.

int readRawPacket(int socket_fd, ethernet_header* ethernet_header, mip_header* mip_header, char* payload, int* interface);
int sendRawPacket(int socket_fd, struct sockaddr_ll* sock_addr, mip_header* mip_header, char* buffer, int len, u_int8_t dst_addr[ETH_ALEN]);
void sendData(int socket_fd, mip_header* mip_header, char* buffer, u_int8_t mip);
#endif
