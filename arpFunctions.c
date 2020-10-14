#include "arpFunctions.h" //Singatures and defintions of this file.
#include "rawFunctions.h" //sendData.
#include "interfaceFunctions.h" //Interfaces.
#include <string.h> //memcpy.
#include <stdio.h>  //printf.
#include <stdbool.h>  //Boolean values.
#include "log.h"  //timestamp.

extern list* interfaces; //Extern linkedList of interfaces from the mip-deamon.
extern list* arpCache; //Extern linkedList of arp-entries from the mip-deamon.
extern bool debug; //Extern boolean flag from the mip-deamon.
extern u_int8_t MY_MIP_ADDRESS; //Extern mip-address from the mip-deamon.



/*
    This file contains functions to perform arp-related operations.
    Such as adding to arp-cache, and fetching enteries and sending
    of arp-requests and arp-braodcasts. There is also functionality
    to store mip-datagrams while waiting for arp-resonses.
*/



/*
    This functions adds another entry into a arp-cache.

    @Param  a mip-address, the corresponding mac-address and the interface related to this entry.
*/
void addArpEntry(u_int8_t mip, u_int8_t mac[ETH_ALEN], int interface)
{
  //Create a arpEntry struct and fill in the fields with the data specified as parameters.
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

    @Params   A mip-address that is to be looked up in the cache.
    @Return  a ArpCache entry, if the mip-address supplied has a corresponding entry, if not NULL.
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
    This function updates the arpEntry in the cache if another host has taken over a mip-address.

    @Param   The mip-address, a new mac-address aswell as the related interface.
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
    This function is called whenever a arp-response arrives.
    The function will send all the mip-datagrams that has been waiting
    for the mac supplied by the arp-response.

    @Param  A raw-socket, a list of waiting entries and the mip-address
    of the datagrams to be extracted and sent from the list.
*/
void sendWaitingMsgs(int socket_fd, list* arpWaitingList, u_int8_t mip)
{
  if(arpWaitingList -> head == NULL)
    return;


  node* tempNode = arpWaitingList -> head;
  bool changed = false;

  while(tempNode != NULL)
  {
    struct arpWaitEntry* current = (struct arpWaitEntry*) tempNode -> data;

    //Found a node with data to be sent.
    if(current -> dst == mip)
     {
       if(tempNode == arpWaitingList -> head)
       {
         arpWaitingList -> head = tempNode -> next;
       }

       //Send the data and free the memory of the node.
       sendData(socket_fd, current -> mip_header, current -> buffer, current -> dst);
       node* temp = tempNode;
       tempNode = tempNode -> next;
       free(temp -> data);
       free(temp);
       changed = true;
       arpWaitingList -> entries = arpWaitingList -> entries -1;
     }

     if(!changed) {tempNode = tempNode -> next;}
     else if(changed) {changed = false;}
   }
}



/*
    This functions frees all memory related to the arpWaitingList.
    If the list is empty, just free the list pointer.
    If not, free all nodes and their data.

    @Param  A linkedList of mip-datagrams waiting for arp-responses.
*/
void freeArpList(list* arpWaitingList)
{
  if(arpWaitingList -> head == NULL)
  {
    freeListMemory(arpWaitingList);
    return;
  }

  node* tempNode = arpWaitingList -> head;

  //Find the node, remove it.
  while(tempNode != NULL)
  {
     struct arpWaitEntry* current = (struct arpWaitEntry*) tempNode -> data;
     free(current -> mip_header);
     free(current -> buffer);
     tempNode = tempNode -> next;
   }
   freeListMemory(arpWaitingList);
}



/*
    The funtion constructs an arp-response and a mip-header and sends it over to
    another funciton along with the neighbour to send this data to.

    @Param  a socket-fd to send the packet over and a destination mip-address.
*/
void sendArpResponse(int socket_fd, u_int8_t dst_mip)
{

  if(debug)
  {
    timestamp();
    printf("SENDING ARP-RESPONSE -- SRC MIP: %d -- DST MIP: %d\n\n", MY_MIP_ADDRESS, dst_mip);
  }

  //Create a pointer to a mip-header and allocate space.
  mip_header* mip_header = calloc(1, sizeof(struct mip_header));
  arpMsg arpMsg; //The SDU.
  memset(&arpMsg, 0, sizeof(arpMsg));
  //Fill in the fields of the mip-header.
  mip_header -> dst_addr = dst_mip;
  mip_header -> src_addr = MY_MIP_ADDRESS;
  mip_header -> ttl = 1; //ttl is one, because of broadcast.
  mip_header -> sdu_length = sizeof(struct arpMsg);
  mip_header -> sdu_type = ARP;

  //Fill in the arpMsg fields.
  arpMsg.type = ARP_RESPONSE;
  arpMsg.mip_address = MY_MIP_ADDRESS;

  //Crate a char* buffer and allocate space equal to the size of a arpMsg, then fill in the buffer.
  char* buffer = calloc(1, sizeof(struct arpMsg));
  memcpy(buffer, &arpMsg, sizeof(struct arpMsg));

  //Send the data over to the function sendData(rawFunctions.c).
  sendData(socket_fd, mip_header, buffer, mip_header -> dst_addr);
}



/*
    The funtion constructs an arp-broadcast and a mip-header and sends it over to
    another funciton along with the node to lookup.

    @Param  a socket-descriptor to send the packet on, a list of interfaces and the mip too lookup.
*/
void sendArpBroadcast(int socket_fd, list* interfaces, u_int8_t lookup)
{
  if(debug)
  {
    timestamp();
    printf("SENDING ARP-BROADCAST -- SRC MIP: %d -- DST MIP: %d -- LOOKUP: %d\n\n", MY_MIP_ADDRESS, 0xFF, lookup);
  }

  uint8_t dst_addr[ETH_ALEN] = BROADCAST_MAC_ADDR; //The destination.

  //Create a pointer to a mip-header and allocate space.
  mip_header* mip_header = calloc(1, sizeof(struct mip_header));
  arpMsg arpMsg; //The SDU.
  memset(&arpMsg, 0, sizeof(arpMsg));

  //Fill in the mip-header.
  mip_header -> dst_addr = 0xFF;
  mip_header -> src_addr = MY_MIP_ADDRESS;
  mip_header -> ttl = 1; //ttl one because of broadcast.
  mip_header -> sdu_length = sizeof(struct arpMsg);
  mip_header -> sdu_type = ARP;

  //Fill in the arpMsg.
  arpMsg.type = ARP_BROADCAST;
  arpMsg.mip_address = lookup;

  //Create a buffer, and copy content of arpMsg into this.
  char* buffer = calloc(1, sizeof(struct arpMsg));
  memcpy(buffer, &arpMsg, sizeof(struct arpMsg));

  //Send the data over to the function sendData(rawFunctions.c).
  sendData(socket_fd, mip_header, buffer, 0xFF);
}
