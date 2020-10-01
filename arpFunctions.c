#include "arpFunctions.h"
#include "rawFunctions.h"
#include "interfaceFunctions.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
extern list* interfaces;
extern u_int8_t MY_MIP_ADDRESS;
extern bool debug;
extern list* arpCache;

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

/*
    This function prints every arpEntry in the cache.
    @Param  a linkedlist (the arpCache).
*/
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

/*
    The funtion constructs an arp response and a mip-header and sends it over to
    another funciton along with the instruction of what interface to send on
    and the destination mac address.

    @param  a socket-descriptor to send the packet on and a destination mip.
*/
void sendArpResponse(int socket_fd, u_int8_t dst_mip)
{
  arpEntry* entry = getCacheEntry(arpCache, dst_mip);
  mip_header mip_header;
  arpMsg arpMsg;
  memset(&mip_header, 0, sizeof(struct mip_header));
  memset(&arpMsg, 0, sizeof(struct arpMsg));

  mip_header.dst_addr = entry -> mip_address;
  mip_header.src_addr = MY_MIP_ADDRESS;
  mip_header.ttl = 1;
  mip_header.sdu_length = sizeof(struct arpMsg);
  mip_header.sdu_type = ARP;

  arpMsg.type = ARP_RESPONSE;
  arpMsg.mip_address = dst_mip;

  char* buffer = malloc(sizeof(struct arpMsg));
  memcpy(buffer, &arpMsg, sizeof(struct arpMsg));
  struct sockaddr_ll temp;
  interface* interface = getInterface(interfaces, entry -> via_interface);
  memcpy(&temp, &interface -> sock_addr, sizeof(struct sockaddr_ll));

  if(debug)
    printf("\n\nSENDING ARP-RESPONSE -- SRC MIP: %d -- DST MIP: %d\n", mip_header.src_addr, mip_header.dst_addr);

  sendRawPacket(socket_fd, &temp, &mip_header, buffer, sizeof(arpMsg), entry -> mac_address);
}

/*
    The funtion constructs an arp broadcast and a mip-header and sends it over to
    another funciton along with the instruction of what interface to send on (all)
    and the destination mac address (broadcast).

    @param  a socket-descriptor to send the packet on, a list of interfaces and the mip too lookup.
*/
void sendArpBroadcast(int socket_fd, list* interfaces, u_int8_t lookup)
{
  mip_header mip_header;
  arpMsg arpMsg;
  memset(&mip_header, 0, sizeof(struct mip_header));
  memset(&arpMsg, 0, sizeof(struct arpMsg));

  uint8_t dst_addr[ETH_ALEN] = BROADCAST_MAC_ADDR;

  mip_header.dst_addr = 0xFF;
  mip_header.src_addr = MY_MIP_ADDRESS;
  mip_header.ttl = 1;
  mip_header.sdu_length = sizeof(struct arpMsg);
  mip_header.sdu_type = ARP;

  arpMsg.type = ARP_BROADCAST;
  arpMsg.mip_address = lookup;

  char* buffer = malloc(sizeof(struct arpMsg));
  memset(buffer, 0, sizeof(struct arpMsg));
  memcpy(buffer, &arpMsg, sizeof(struct arpMsg));


  node* tempInterface = interfaces -> head;
  struct sockaddr_ll temp;

  while(tempInterface != NULL)
  {
    interface* currentInterface = (struct interface*) tempInterface -> data;
    memcpy(&temp, &currentInterface -> sock_addr, sizeof(struct sockaddr_ll));
    if(debug)
      printf("\n\nSENDING ARP-BROADCAST -- SRC MIP: %d -- DST MIP: %d\n", MY_MIP_ADDRESS, 0xFF);
    sendRawPacket(socket_fd, &temp, &mip_header, buffer, sizeof(arpMsg), dst_addr);
    tempInterface = tempInterface -> next;
  }
  free(buffer);
}
