#ifndef ROUTING_
#define ROUTING_

//Routing related codes
#define REQ {0x52, 0x45, 0x51}
#define RSP {0x52, 0x53, 0x50}
#define UPD {0x55, 0x50, 0x44}
#define HEL {0x48, 0x45, 0x4c}
#define INTERNAL 0x00
#define REMOTE 0x01


typedef struct routingMsgIntra msgIntra;
typedef struct routingEntry routingEntry;

struct routingMsgIntra
{
  u_int8_t type[3];
  u_int8_t mip_other;
};

struct routingEntry
{
  u_int8_t mip_address;
  u_int8_t cost;
  u_int8_t next_hop;
};

#endif
