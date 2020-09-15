#include "arpFunctions.h"

void addArpEntry(list* arpCache, u_int8_t mip, u_int8_t mac[ETH_ALEN], int interface)
{
  arpEntry entry;
  entry.mip_address = mip;
  memcpy(entry.mac_address, mac, ETH_ALEN);
  memcpy(&entry.via_interface, &interface, sizeof(int));
  addEntry(arpCache, &entry);
}

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

void updateArpEntry(list* arpCache, uint8_t mip, u_int8_t mac[ETH_ALEN], int interface)
{
  arpEntry* entry = getCacheEntry(arpCache, mip);
  memcpy(entry -> mac_address, mac, ETH_ALEN);
  entry -> via_interface = interface;
}
