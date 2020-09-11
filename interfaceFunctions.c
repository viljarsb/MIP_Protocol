#include "interfaceFunctions.h"

void findInterfaces(list* interfaceList)
{
  struct ifaddrs *ifaces, *ifp;
  if (getifaddrs(&ifaces)) {
     perror("getifaddrs");
     exit(-1);
   }

  for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next)
  {
      if (ifp->ifa_addr != NULL && ifp->ifa_addr->sa_family == AF_PACKET && strcmp("lo", ifp->ifa_name) != 0)
      {
        interface interface;
        memcpy(&interface.sock_addr, (struct sockaddr_ll*) ifp->ifa_addr, sizeof(struct sockaddr_ll));
        addEntry(interfaceList, &interface);
      }
  }
  /* Free the interface list */
  freeifaddrs(ifaces);
  return;
}

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
