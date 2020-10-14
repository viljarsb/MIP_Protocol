#include <ifaddrs.h> //Declarations of getting network interface addresses.
#include <netinet/ether.h> //Ethernet declarations.
#include <stdlib.h> //u_int8_t.
#include <stdio.h> //printf.
#include <string.h> //memcpy.
#include <stdbool.h> //Boolean values.
#include "interfaceFunctions.h" //Signatures of this file.
#include "log.h" //timestamp

extern bool debug; //Extern debug flag.



/*
    This file provides utility functions to find, fetch and use network interfaces
*/



/*
    This function finds all the active interfaces on the current host (Except for loopback device)
    The function then adds them all to a linkedlist.

    @Param  A linkedlist to add all the interfaces to.
*/
void findInterfaces(list* interfaceList)
{
  if(debug)
  {
    timestamp();
    printf("LOOKING FOR ACTIVE INTERFACES ON THIS NODE\n");
  }

  //Find all devices.
  struct ifaddrs *ifaces, *ifp;
  if(getifaddrs(&ifaces))
  {
     printf("getifaddrs\n");
     exit(EXIT_FAILURE);
  }

  //Add devices to list.
  for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next)
  {
      if (ifp->ifa_addr != NULL && ifp->ifa_addr->sa_family == AF_PACKET && strcmp("lo", ifp->ifa_name) != 0)
      {
        interface interface;
        memcpy(&interface.sock_addr, (struct sockaddr_ll*) ifp->ifa_addr, sizeof(struct sockaddr_ll));
        interface.name = malloc(strlen(ifp -> ifa_name) + 1);
        memcpy(interface.name, ifp -> ifa_name, strlen(ifp -> ifa_name) + 1);
        if(debug)
        {
          timestamp();
          printf("ADDING INTERFACE No. %d -- NAME %s -- TO ACTIVE INTERFACES\n", interface.sock_addr.sll_ifindex, interface.name);
        }
        addEntry(interfaceList, &interface);
      }
  }

  printf("\n");

  //Free the interface list.
  freeifaddrs(ifaces);
}



/*
    This function looks trough the list of interfaces and finds the interface that is requested.

    @Param  A linkedlist (list of interfaces) and a int (the interface we want)
    @Return  A pointer to a interface if found, if not a NULL.
*/
interface* getInterface(list* interfaceList, int interface)
{
  if(interfaceList -> head == NULL)
    return NULL;

  node* tempNode = interfaceList -> head;
  while(tempNode != NULL)
  {
     struct interface* currentInterface = (struct interface*) tempNode -> data;
     if (currentInterface -> sock_addr.sll_ifindex == interface) {
         return currentInterface;
     }
     tempNode = tempNode -> next;
 }

 return NULL;
}



/*
    This functions returns a mac_address in a more desirable format.

    @Param  A mac_address (array of size ETH_ALEN).
    @Return  A string of the mac_address in a nice format.
*/
char* getMacFormat(u_int8_t mac[ETH_ALEN])
{
  return ether_ntoa((struct ether_addr*)mac);
}



/*
    This functions frees memory from interface structs in a list.

    @Param  a linkedlist of interfaces.
*/
void freeInterfaces(list* interfaceList)
{
  if(interfaceList -> head == NULL)
    return;

  node* tempNode = interfaceList -> head;
  while(tempNode != NULL)
  {
     struct interface* currentInterface = (struct interface*) tempNode -> data;
     free(currentInterface -> name);
     tempNode = tempNode -> next;
   }

 freeListMemory(interfaceList);
}
