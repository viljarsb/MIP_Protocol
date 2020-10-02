#ifndef PROTOCOL
#define PROTOCOL
#include <stdlib.h>
#include <net/ethernet.h>


typedef struct ethernet_header ethernet_header;
typedef struct mip_header mip_header;
typedef struct arpMsg arpMsg;

#define ROUTING 0x04
#define PING 0x02
#define ARP 0x01
#define ETH_P_MIP 0x88B5
#define DEFAULT_TTL 10;
#define BROADCAST_MAC_ADDR {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}

struct ethernet_header {
    u_int8_t dst_addr[ETH_ALEN];
    u_int8_t src_addr[ETH_ALEN];
    u_int16_t protocol;
}__attribute__ ((packed));

struct mip_header {
  u_int8_t dst_addr;
  u_int8_t src_addr;
  u_int8_t ttl:4;
  u_int16_t sdu_length:9;
  u_int8_t sdu_type:3;
}__attribute__((packed));

struct arpMsg {
  u_int8_t type:1;
  u_int8_t mip_address;
  u_int32_t padding:23;
}__attribute__((packed));

#endif
