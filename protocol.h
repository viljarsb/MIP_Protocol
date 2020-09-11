#ifndef PROTOCOL
#define PROTOCOL

#include <stdint.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <stdlib.h>

typedef struct ethernet_header ethernet_header;
typedef struct mip_header mip_header;
typedef struct arpMsg arpMsg;
#define PING 0x02
#define ARP 0x01
#define ETH_P_MIP 0x88B5
#define BROADCAST_MAC_ADDR {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}

struct ethernet_header {
    uint8_t dst_addr[ETH_ALEN];
    uint8_t src_addr[ETH_ALEN];
    uint16_t protocol;
} __attribute__ ((packed));

struct mip_header {
  u_int8_t dst_addr;
  u_int8_t src_addr;
  u_int8_t ttl:4;
  u_int16_t sdu_length:9;
  u_int8_t sdu_type:3;
} __attribute__((packed));

struct arpMsg {
  u_int8_t type:1;
  u_int8_t mip_address;
  u_int32_t padding:23;
}__attribute__((packed));


#endif
