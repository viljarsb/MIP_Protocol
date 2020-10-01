#ifndef ARPFUNCTIONS
#define ARPFUNCTIONS
#include "linkedList.h"
#include "applicationFunctions.h"
#include <net/ethernet.h>
#include "protocol.h"
#define ARP_BROADCAST 0x00
#define ARP_RESPONSE 0x01
typedef struct arpEntry arpEntry;

struct arpEntry
{
  uint8_t mip_address;
  uint8_t mac_address[6];
  int via_interface;
}__attribute__ ((packed));

struct arpWaitEntry
{
  u_int8_t dst;
  mip_header* mip_header;
  char* buffer;
};

void addArpEntry(list* arpCache, u_int8_t mip, u_int8_t mac[ETH_ALEN], int interface);
arpEntry* getCacheEntry(list* arpCache, uint8_t mip);
void updateArpEntry(list* arpCache, uint8_t mip, u_int8_t mac[ETH_ALEN], int interface);
void printArpCache(list* arpCache);
void sendArpResponse(int socket_fd, u_int8_t dst_mip);
void sendArpBroadcast(int socket_fd, list* interfaces, u_int8_t lookup);

#endif
