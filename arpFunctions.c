#include "arpFunctions.h"
#include "interfaceFunctions.h"
#include <string.h>
#include <stdio.h>
extern list* interfaces;

/*
    This functions adds another entry into a arp cache.
    @Param  a linkedlist (the arpCache), the mip and the corresponding mac and the interface related to this entry.
*/
void addArpEntry(list* arpCache, u_int8_t mip, u_int8_t mac[ETH_ALEN], int interface)
{
  arpEntry entry;
  entry.mip_address = mip;
  memcpy(entry.mac_address, mac, ETH_ALEN);
  memcpy(&entry.via_interface, &interface, sizeof(int));
  addEntry(arpCache, &entry);
}

/*
    This function fetches an entry from the arpCache.
    @Params   a linkedlist (the arpCache) and the MIP that is looked up in the cache.
    @Return  a ArpCache entry if the MIP supplied has a corresponding arp entry, if not NULL.

*/
arpEntry* getCacheEntry(list* arpCache, uint8_t mip) {
    if(arpCache -> head == NULL)
    {
      return NULL;
    }

    node* tempNode = arpCache -> head;
    while(tempNode != NULL)
    {
        arpEntry* entry = (arpEntry*) tempNode -> data;
        if (entry -> mip_address == mip)
        {
            return entry;
        }
      tempNode = tempNode -> next;
    }
    return NULL;
}

/*
    This function updates the arpEntry in the cache if another host has taken over a MIP.
    @Param  a linkedlist (the arpCache), a new mip and a new mac aswell as the related interface.

*/
void updateArpEntry(list* arpCache, uint8_t mip, u_int8_t mac[ETH_ALEN], int interface)
{
  arpEntry* entry = getCacheEntry(arpCache, mip);
  memcpy(entry -> mac_address, mac, ETH_ALEN);
  entry -> via_interface = interface;
}

void printArpCache(list* arpCache)
{
  if(arpCache -> head == NULL)
  {
    printf("\n\nARP-CACHE IS CURRENLY EMPTY\n");
    return;
  }
  printf("\n\nPRINTING CURRENT CACHE ENTRIES\n");

  node* tempNode = arpCache -> head;
  while(tempNode != NULL)
  {
      arpEntry* entry = (arpEntry*) tempNode -> data;
      char* mac = getMacFormat(entry -> mac_address);
      interface* interface = getInterface(interfaces, entry -> via_interface);
      printf("CACHE ENTRY -- MIP: %d -- MAC: %s -- INTERFACE: %d %s\n", entry -> mip_address, mac, entry -> via_interface, interface -> name);
    tempNode = tempNode -> next;
  }
  return;
}
