#ifndef ARPFUNCTIONS
#define ARPFUNCTIONS
#include "protocol.h"
#include "linkedList.h"
typedef struct arpEntry arpEntry;

struct arpEntry
{
  uint8_t mip_address;
  uint8_t mac_address[6];
  int via_interface;
}__attribute__ ((packed));

void addArpEntry(list* arpCache, u_int8_t mip, u_int8_t mac[ETH_ALEN], int interface);
arpEntry* getCacheEntry(list* arpCache, uint8_t mip);

#endif
