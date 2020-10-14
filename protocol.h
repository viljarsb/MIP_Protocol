#ifndef PROTOCOL
#define PROTOCOL
#include <stdlib.h> //u_int8_t.
#include <net/ethernet.h> //ETH_ALEN.

typedef struct ethernet_header ethernet_header;
typedef struct mip_header mip_header;
typedef struct arpMsg arpMsg;

//Supported SDU-CODES.
#define ROUTING 0x04
#define PING 0x02
#define ARP 0x01

//ETHER-TYPE OF MIP.
#define ETH_P_MIP 0x88B5

//Some predefined values.
#define DEFAULT_TTL_MIP 15; //Max jumps is 15, because of 4-bit ttl in mip-header.
#define BROADCAST_TTL_MIP 1; //TTL of broadcasts will always be 1.
#define BROADCAST_MAC_ADDR {0xff, 0xff, 0xff, 0xff, 0xff, 0xff} //THe broadcast mac-address.

//Ethernet-header struct, holds destination, source and the ethertype.
struct ethernet_header {
    u_int8_t dst_addr[ETH_ALEN];
    u_int8_t src_addr[ETH_ALEN];
    u_int16_t protocol;
}__attribute__ ((packed));

//Mip-header sturct, holdt destination, source, ttl, sdu length and the sdu type.
struct mip_header {
  u_int8_t dst_addr;
  u_int8_t src_addr;
  u_int8_t ttl:4;
  u_int16_t sdu_length:9;
  u_int8_t sdu_type:3;
}__attribute__((packed));


#endif
