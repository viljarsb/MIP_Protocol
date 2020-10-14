#ifndef InterfaceFunctions
#define InterfaceFunctions

#include <linux/if_packet.h> //sockaddr_ll struct.
#include "linkedList.h" //linkedList.

typedef struct interface interface;

//A struct for storing info about a interface.
struct interface
{
  struct sockaddr_ll sock_addr;
  char* name;
};

void findInterfaces(list* interfaceList);
interface* getInterface(list* interfaceList, int interface);
char* getMacFormat(u_int8_t mac[ETH_ALEN]);
void freeInterfaces(list* interfaceList);
#endif
