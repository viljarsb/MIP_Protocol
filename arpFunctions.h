#ifndef ARPFUNCTIONS
#define ARPFUNCTIONS
#include <net/ethernet.h>
#include "linkedList.h"
#include "protocol.h"

#define ARP_BROADCAST 0x00
#define ARP_RESPONSE 0x01
typedef struct arpEntry arpEntry;

struct arpEntry
{
  uint8_t mip_address;
  uint8_t mac_address[ETH_ALEN];
  int via_interface;
}__attribute__ ((packed));

struct arpWaitEntry
{
  u_int8_t dst;
  mip_header* mip_header;
  char* buffer;
};

void addArpEntry(u_int8_t mip, u_int8_t mac[ETH_ALEN], int interface);
arpEntry* getCacheEntry(uint8_t mip);
void updateArpEntry(uint8_t mip, u_int8_t mac[ETH_ALEN], int interface);
void printArpCache();
void sendArpResponse(int socket_fd, u_int8_t dst_mip);
void sendArpBroadcast(int socket_fd, list* interfaces, u_int8_t lookup);

#endif
