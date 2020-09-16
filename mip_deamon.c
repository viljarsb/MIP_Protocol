#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <signal.h>

#include "mip_deamon.h"
#include "arpFunctions.h"
#include "protocol.h"
#include "interfaceFunctions.h"
#include "rawFunctions.h"
#include "socketFunctions.h"
#include "epollFunctions.h"


u_int8_t MY_MIP_ADDRESS;
int debug = 0;
list* interfaces;
list* arpCache;
applicationMsg* unsent = NULL;

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

  arpMsg.type = 0x01;
  arpMsg.mip_address = dst_mip;

  char* buffer = malloc(sizeof(struct arpMsg));
  memcpy(buffer, &arpMsg, sizeof(struct arpMsg));
  struct sockaddr_ll temp;
  interface* interface = getInterface(interfaces, entry -> via_interface);
  memcpy(&temp, &interface -> sock_addr, sizeof(struct sockaddr_ll));

  if(debug == 1)
    printf("\n\nSENDING ARP-RESPONSE -- SRC MIP: %d -- DST MIP: %d\n", mip_header.src_addr, mip_header.dst_addr);
  sendRawPacket(socket_fd, &temp, &mip_header, buffer, sizeof(struct arpMsg), entry -> mac_address);
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
  uint8_t dst_addr[ETH_ALEN] = BROADCAST_MAC_ADDR;
  memset(&mip_header, 0, sizeof(struct mip_header));
  memset(&arpMsg, 0, sizeof(struct arpMsg));

  mip_header.dst_addr = 0xFF;
  mip_header.src_addr = MY_MIP_ADDRESS;
  mip_header.ttl = 1;
  mip_header.sdu_length = sizeof(struct arpMsg);
  mip_header.sdu_type = ARP;

  arpMsg.type = 0x00;
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
    if(debug == 1)
    {
      printf("\n\nSENDING ARP-BROADCAST -- SRC MIP: %d -- DST MIP: %d\n", MY_MIP_ADDRESS, 0xFF);
    }
    sendRawPacket(socket_fd, &temp, &mip_header, buffer, sizeof(struct arpMsg), dst_addr);
    tempInterface = tempInterface -> next;
  }
  free(buffer);
}

/*
    The funtion constructs a mip-header with a ping-sdu and sends it over to
    another funciton along with the instruction of what interface to send on
    (according to the cache) and the destination mac address (according to cache).

    If the destination mip is not cached, the functions calls the send arp broadcast
    funtion to first obtain a correct mac to send to.

    @param  a socket-descriptor to send the packet on, the ping msg, a list of interfaes and an arp-cache.
*/
void sendApplicationData(int socket_fd, applicationMsg* msg, u_int8_t mipDst)
{
  arpEntry* entry = getCacheEntry(arpCache, msg -> address);
  if(entry == NULL)
  {
    sendArpBroadcast(socket_fd, interfaces, msg -> address);
    unsent = calloc(1, sizeof(struct applicationMsg));
    memcpy(&unsent -> address, &msg -> address, sizeof(u_int8_t));
    memcpy(unsent -> payload, msg -> payload, strlen(msg -> payload));
    return;
  }

  mip_header mip_header;
  memset(&mip_header, 0, sizeof(struct mip_header));
  mip_header.src_addr = MY_MIP_ADDRESS;
  mip_header.dst_addr = mipDst;
  mip_header.sdu_type = PING;
  mip_header.ttl = 1;
  mip_header.sdu_length = strlen(msg -> payload);


  interface* interfaceToUse = getInterface(interfaces, entry -> via_interface);
  struct sockaddr_ll temp;
  memcpy(&temp, &interfaceToUse -> sock_addr, sizeof(struct sockaddr_ll));
  if(debug == 1)
  {
    printf("\n\nSENDING PING -- MIP SRC: %d -- MIP DST: %d\n", mip_header.src_addr, mip_header.dst_addr);
  }
  sendRawPacket(socket_fd, &temp, &mip_header, msg -> payload, strlen(msg -> payload), entry -> mac_address);
}

/*
    Function read and inspects incoming raw packets.
    The function may send out arp responses to broadcasts.
    The function may also cache addresses based on incoming responses.
    The function can also forward msgs to the current active application in case it reads a ping.

    @Param a raw socket and the current active application socket.
*/
void handleRawPacket(int socket_fd, int activeApplication)
{
    ethernet_header ethernet_header;
    mip_header mip_header;

    memset(&ethernet_header, 0, sizeof(struct ethernet_header));
    memset(&mip_header, 0, sizeof(struct mip_header));
    char* buffer = malloc(1024);
    memset(buffer, 0, 1024);

    int* recivedOnInterface = malloc(sizeof(int));
    readRawPacket(socket_fd, &ethernet_header, &mip_header, buffer, recivedOnInterface);
    if(mip_header.sdu_type == ARP)
    {
      struct arpMsg arpMsg;
      memcpy(&arpMsg, buffer, sizeof(struct arpMsg));

      //broadcast
       if(mip_header.dst_addr == 0xFF)
       {
         if(arpMsg.type == 0x00 && arpMsg.mip_address == MY_MIP_ADDRESS)
         {
           if(getCacheEntry(arpCache, mip_header.src_addr) == NULL)
           {
             addArpEntry(arpCache, mip_header.src_addr, ethernet_header.src_addr, *recivedOnInterface);
           }
           else
           {
             updateArpEntry(arpCache, mip_header.src_addr, ethernet_header.src_addr, *recivedOnInterface);

           }
           if(debug == 1)
           {
             printf("RECEIVED ARP-REQUEST -- MIP SRC: %d -- MIP DST: %d\n", mip_header.src_addr, mip_header.dst_addr);
             printArpCache(arpCache);
           }
           sendArpResponse(socket_fd, mip_header.src_addr);
         }
       }

       //response
       else if(mip_header.dst_addr == MY_MIP_ADDRESS)
       {
         if(arpMsg.type == 0x01)
         {
            addArpEntry(arpCache, mip_header.src_addr, ethernet_header.src_addr, *recivedOnInterface);

           if(debug == 1)
           {
             printf("RECEIVED ARP-RESPONSE -- SRC MIP: %d -- DST MIP: %d \n", mip_header.src_addr, mip_header.dst_addr);
             printArpCache(arpCache);
           }

            if(unsent != NULL)
            {
              sendApplicationData(socket_fd, unsent, unsent -> address);
              free(unsent);
              unsent = NULL;
            }
         }
       }
    }

    //ping
    else if(mip_header.sdu_type == PING)
    {
      if(debug == 1)
      {
        printf("RECEIVED PING -- MIP SRC: %d -- MIP DST: %d\n", mip_header.src_addr, mip_header.dst_addr);
      }
      applicationMsg msg;
      memset(&msg, 0, sizeof(struct applicationMsg));
      memcpy(&msg.address, &mip_header.src_addr, sizeof(u_int8_t));
      memcpy(msg.payload, buffer, strlen(buffer));
      sendApplicationMsg(activeApplication, msg.address, msg.payload);
    }

    free(buffer);
    free(recivedOnInterface);
  }

/*
      Function read and inspects incoming application packets.
      The function may send the msg back to the application in case it was addressed to itself.
      If not the msg is passed on to the send ping function.

      @Param a application socket and a raw socket.
*/
void handleApplicationPacket(int activeApplication, int socket_fd)
{
  applicationMsg msg = readApplicationMsg(activeApplication);

  if(msg.address == MY_MIP_ADDRESS)
  {
    if(debug == 1)
      printf("AN APPLICATION ON THIS HOST ADDRESS A PACKET TO IT SELF. SENDING THE MSG IN RETURN\n");
    sendApplicationMsg(activeApplication, msg.address, msg.payload);
  }

  else if(msg.address != MY_MIP_ADDRESS)
  {
    sendApplicationData(socket_fd, &msg, msg.address);
  }
}

//Very simple signal handler.
void handle_sigint(int sig)
{
    printf("MIP_DEAMON FORCE-QUIT\n");
    void freeInterfaces(interfaces);
    freeListMemory(arpCache);
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
  char* domainPath;

    if(argc == 4 && strcmp(argv[1], "-d") == 0)
    {
      debug = 1;
      domainPath = argv[2];
      MY_MIP_ADDRESS = atoi(argv[3]);
    }

    else if(argc == 2 && strcmp(argv[1], "-h") == 0)
    {
      printf("Run program with -d (optional debugmode), <domain path> <mip address>\n");
      exit(EXIT_SUCCESS);
    }

    else if(argc == 3)
    {
      domainPath = argv[1];
      MY_MIP_ADDRESS = atoi(argv[2]);
    }

    else
    {
      printf("Run program with -h as argument for instructions\n");
      exit(EXIT_SUCCESS);
    }

    interfaces = createLinkedList(sizeof(interface));
    findInterfaces(interfaces);
    arpCache = createLinkedList(sizeof(arpEntry));
    int socket_fd, socket_unix;

    socket_unix = createDomainServerSocket(domainPath);
    socket_fd = createRawSocket();

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
       perror("epoll_create");
       exit(EXIT_FAILURE);
     }

    struct epoll_event events[10];
    int amountOfEntries;
    addEpollEntry(socket_unix, epoll_fd);
    addEpollEntry(socket_fd, epoll_fd);
    int activeApplication = -1;

    while(true)
    {
      amountOfEntries = epoll_wait(epoll_fd, events, 10, -1);
      signal(SIGINT, handle_sigint);
       for (int i = 0; i < amountOfEntries; i++)
       {
         if(events[i].data.fd == socket_unix)
         {
           activeApplication = accept(socket_unix, NULL, NULL);
           addEpollEntry(activeApplication, epoll_fd);
         }

         //closing
         else if(events[i].events & EPOLLHUP)
         {
           removeEpollEntry(activeApplication, epoll_fd);
           close(activeApplication);
           activeApplication = -1;
         }

         //data
         else if (events[i].events & EPOLLIN)
         {
             if(events[i].data.fd == activeApplication)
             {
               handleApplicationPacket(activeApplication, socket_fd);
             }

             else if(events[i].data.fd == socket_fd)
             {
               handleRawPacket(socket_fd, activeApplication);
             }
         }
       }
    }
  }
