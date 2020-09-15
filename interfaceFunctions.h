#ifndef InterfaceFunctions
#define InterfaceFunctions

#include <net/ethernet.h>
#include <linux/if_packet.h>
#include "linkedList.h"

typedef struct interface interface;

struct interface
{
  struct sockaddr_ll sock_addr;
  char* name;
};

void findInterfaces(list* interfaceList);
interface* getInterface(list* interfaceList, int interface);
char* getMacFormat(u_int8_t mac[ETH_ALEN]);



#endif
