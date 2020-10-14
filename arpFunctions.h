#ifndef ARPFUNCTIONS
#define ARPFUNCTIONS
#include <net/ethernet.h> //ETH_ALEN.
#include "linkedList.h" //linkedList.
#include "protocol.h" //mip_header defintion.

//Arp codes.
#define ARP_BROADCAST 0x00
#define ARP_RESPONSE 0x01

typedef struct arpEntry arpEntry;

//A struct for storing data of mip, corresponding mac and the interface in a cache table.
struct arpEntry
{
  uint8_t mip_address;
  uint8_t mac_address[ETH_ALEN];
  int via_interface;
}__attribute__ ((packed));

//A struct for storing destination mip, pointer to mip-header and the payload of mip-datagrams
//waiting to be sent until a arp-respones arrives.
struct arpWaitEntry
{
  u_int8_t dst;
  mip_header* mip_header;
  char* buffer;
};

//Struct of a arpMsg, holdt the type, a mip and padding.
struct arpMsg {
  u_int8_t type:1;
  u_int8_t mip_address;
  u_int32_t padding:23;
}__attribute__((packed));

void sendWaitingMsgs(int socket_fd, list* arpWaitingList, u_int8_t mip);
void freeArpList(list* arpWaitingList);
void addArpEntry(u_int8_t mip, u_int8_t mac[ETH_ALEN], int interface);
arpEntry* getCacheEntry(uint8_t mip);
void updateArpEntry(uint8_t mip, u_int8_t mac[ETH_ALEN], int interface);
void printArpCache();
void sendArpResponse(int socket_fd, u_int8_t dst_mip);
void sendArpBroadcast(int socket_fd, list* interfaces, u_int8_t lookup);

#endif
