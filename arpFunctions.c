#include "arpFunctions.h"
#include "rawFunctions.h" //sendApplicationData
#include "interfaceFunctions.h" //Interfaces.
#include <string.h> //Memcpy.
#include <stdio.h>  //printf.
#include <stdbool.h>  //Boolean values.
#include "log.h"  //timestamp.

extern list* interfaces; //Extern linkedList of interfaces from the mip-deamon.
extern list* arpCache; //Extern linkedList of arp-entries from the mip-deamon.
extern bool debug; //Extern boolean flag from the mip-deamon.
extern u_int8_t MY_MIP_ADDRESS; //Extern mip-address from the mip-deamon.


/*
    This functions adds another entry into a arp-cache.
    @Param  a linkedlist (the arp-cache), the mip and the corresponding mac and the interface related to this entry.
*/
void addArpEntry(u_int8_t mip, u_int8_t mac[ETH_ALEN], int interface)
{
  //Create a arpEntry struct and fill t he fields with the data specified as parameters.
  arpEntry entry;
  memcpy(&entry.mip_address, &mip, sizeof(mip));
  memcpy(entry.mac_address, mac, ETH_ALEN);
  memcpy(&entry.via_interface, &interface, sizeof(interface));

  if(debug)
  {
    timestamp();
    printf("ADDED %d TO ARP-CACHE -- MAC: %s -- INTERFACE %d\n", entry.mip_address, getMacFormat(entry.mac_address), entry.via_interface);
  }
  //Add to the cache specified.
  addEntry(arpCache, &entry);
}

/*
    This function fetches an entry from the arp-cache.
    @Params   a linkedlist (the arp-cache) and the MIP that is to be looked up in the cache.
    @Return  a ArpCache entry if the MIP supplied has a corresponding arp entry, if not NULL.
*/
arpEntry* getCacheEntry(uint8_t mip)
{
  //If cache is empty return NULL.
  if(arpCache -> head == NULL)
  {
    return NULL;
  }

  //Look for the correct entry in the list, return it if found.
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
  //If not found, return NULL.
  return NULL;
}

/*
    This function updates the arpEntry in the cache if another host has taken over a MIP.
    @Param  a linkedlist (the arp-cache), a new mip and a new mac aswell as the related interface.
*/
void updateArpEntry(uint8_t mip, u_int8_t mac[ETH_ALEN], int interface)
{
  //Get the correct entry to update.
  arpEntry* entry = getCacheEntry(mip);

  //Update the fields.
  memcpy(entry -> mac_address, mac, ETH_ALEN);
  entry -> via_interface = interface;

  if(debug)
  {
    timestamp();
    printf("UPDATED %d IN ARP-CACHE -- MAC: %s -- INTERFACE %d\n", entry -> mip_address, getMacFormat(entry -> mac_address), entry -> via_interface);
  }
}

/*
    This function prints every entry in the arp-cache.
    @Param  a linkedlist (the arpCache).
*/
void printArpCache()
{
  if(arpCache -> head == NULL)
  {
    timestamp();
    printf("ARP-CACHE IS CURRENLY EMPTY\n\n");
    return;
  }

  timestamp();
  printf("PRINTING CURRENT ARP-CACHE ENTRIES\n");

  node* tempNode = arpCache -> head;
  while(tempNode != NULL)
  {
    arpEntry* entry = (arpEntry*) tempNode -> data;
    char* mac = getMacFormat(entry -> mac_address);
    interface* interface = getInterface(interfaces, entry -> via_interface);
    printf("CACHE ENTRY -- MIP: %d -- MAC: %s -- INTERFACE ID: %d -- INTERFACE-NAME: %s\n", entry -> mip_address, mac, entry -> via_interface, interface -> name);
    tempNode = tempNode -> next;
  }

  printf("\n");
}

/*
    The funtion constructs an arp-response and a mip-header and sends it over to
    another funciton along with the next-node to send this data to.

    @param  a socket-fd to send the packet on and a destination mip.
*/
void sendArpResponse(int socket_fd, u_int8_t dst_mip)
{
  if(debug)
  {
    timestamp();
    printf("SENDING ARP-RESPONSE -- SRC MIP: %d -- DST MIP: %d\n\n", MY_MIP_ADDRESS, dst_mip);
  }

  //Create a pointer to a mip-header and allocate space and a arpMsg struct.
  mip_header* mip_header = calloc(1, sizeof(struct mip_header));
  arpMsg arpMsg;

  //Fill in the fields of the mip-header.
  mip_header -> dst_addr = dst_mip;
  mip_header -> src_addr = MY_MIP_ADDRESS;
  mip_header -> ttl = 1;
  mip_header -> sdu_length = sizeof(struct arpMsg);
  mip_header -> sdu_type = ARP;

  //Fill in the arpMsg fields.
  arpMsg.type = ARP_RESPONSE;
  arpMsg.mip_address = MY_MIP_ADDRESS;

  //Crate a char* buffer and allocate space equal to the size of a arpMsg, then fill in the buffer.
  char* buffer = calloc(1, sizeof(struct arpMsg));
  memcpy(buffer, &arpMsg, sizeof(struct arpMsg));

  //Send the data over to the function sendApplicationData(rawFunctions.c).
  sendApplicationData(socket_fd, mip_header, buffer, mip_header -> dst_addr);
}

/*
    The funtion constructs an arp-broadcast and a mip-header and sends it over to
    another funciton along with the node to lookup.

    @param  a socket-descriptor to send the packet on, a list of interfaces and the mip too lookup.
*/
void sendArpBroadcast(int socket_fd, list* interfaces, u_int8_t lookup)
{
  if(debug)
  {
    timestamp();
    printf("SENDING ARP-BROADCAST -- SRC MIP: %d -- DST MIP: %d -- LOOKUP: %d\n\n", MY_MIP_ADDRESS, 0xFF, lookup);
  }

  //Create a pointer to a mip-header and allocate space and a arpMsg struct.
  mip_header* mip_header = calloc(1, sizeof(struct mip_header));
  arpMsg arpMsg;
  uint8_t dst_addr[ETH_ALEN] = BROADCAST_MAC_ADDR;

  //Fill in the mip-header.
  mip_header -> dst_addr = 0xFF;
  mip_header -> src_addr = MY_MIP_ADDRESS;
  mip_header -> ttl = 1;
  mip_header -> sdu_length = sizeof(struct arpMsg);
  mip_header -> sdu_type = ARP;

  //Fill in the arpMsg.
  arpMsg.type = ARP_BROADCAST;
  arpMsg.mip_address = lookup;

  //Create a buffer, and copy content of arpMsg into this.
  char* buffer = calloc(1, sizeof(struct arpMsg));
  memcpy(buffer, &arpMsg, sizeof(struct arpMsg));

  //Send the data over to the function sendApplicationData(rawFunctions.c).
  sendApplicationData(socket_fd, mip_header, buffer, 0xFF);
}
