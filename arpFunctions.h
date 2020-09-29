#ifndef ARPFUNCTIONS
#define ARPFUNCTIONS
#include "linkedList.h"
#include "applicationFunctions.h"
#include <net/ethernet.h>
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
  applicationMsg* appMsg;
};

void addArpEntry(list* arpCache, u_int8_t mip, u_int8_t mac[ETH_ALEN], int interface);
arpEntry* getCacheEntry(list* arpCache, uint8_t mip);
void updateArpEntry(list* arpCache, uint8_t mip, u_int8_t mac[ETH_ALEN], int interface);
void printArpCache(list* arpCache);

#endif
