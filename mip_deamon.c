#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include "stdbool.h"

#include "interfaceFunctions.h"
#include "linkedList.h"
#include "protocol.h"
#include "epollFunctions.h"
#include "socketFunctions.h"
#include "rawFunctions.h"
#include "arpFunctions.h"
#include "applicationFunctions.h"

u_int8_t myMip = 100;
int debug = true;

void sendArpResponse(int socket_fd, list* interfaces, list* cache, u_int8_t dst_mip)
{
  arpEntry* entry = getCacheEntry(cache, dst_mip);

  mip_header mip_header;
  arpMsg arpMsg;
  memset(&mip_header, 0, sizeof(struct mip_header));
  memset(&arpMsg, 0, sizeof(struct arpMsg));

  mip_header.dst_addr = entry -> mip_address;
  mip_header.src_addr = myMip;
  mip_header.ttl = 1;
  mip_header.sdu_length = sizeof(struct arpMsg);
  mip_header.sdu_type = 0x01;

  arpMsg.type = 0x01;
  arpMsg.mip_address = dst_mip;

  char* buffer = malloc(sizeof(struct arpMsg));
  memcpy(buffer, &arpMsg, sizeof(struct arpMsg));
  struct sockaddr_ll temp;
  interface* interface = getInterface(interfaces, entry -> via_interface);
  memcpy(&temp, &interface -> sock_addr, sizeof(struct sockaddr_ll));
  sendRawPacket(socket_fd, &temp, mip_header, buffer, sizeof(struct arpMsg), entry -> mac_address);
  printf("SENDING ARP-RESPONSE TO FROM MIP: %d TO %d\n", mip_header.src_addr, mip_header.dst_addr);
}

void sendArpBroadcast(int socket_fd, list* interfaces, u_int8_t lookup)
{
  mip_header mip_header;
  arpMsg arpMsg;
  uint8_t dst_addr[ETH_ALEN] = BROADCAST_MAC_ADDR;
  memset(&mip_header, 0, sizeof(struct mip_header));
  memset(&arpMsg, 0, sizeof(struct arpMsg));

  mip_header.dst_addr = 0xFF;
  mip_header.src_addr = myMip;
  mip_header.ttl = 1;
  mip_header.sdu_length = sizeof(struct arpMsg);
  mip_header.sdu_type = 0x01;

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
    sendRawPacket(socket_fd, &temp, mip_header, buffer, sizeof(struct arpMsg), dst_addr);
    tempInterface = tempInterface -> next;
  }
}

void handleRawPacket(int socket_fd, int activeApplication, list* interfaces, list* cache)
{
    ethernet_header ethernet_header;
    mip_header mip_header;

    memset(&ethernet_header, 0, sizeof(struct ethernet_header));
    memset(&mip_header, 0, sizeof(struct mip_header));
    char* buffer = malloc(1024);

    int* recivedOnInterface = malloc(sizeof(int));
    readRawPacket(socket_fd, &ethernet_header, &mip_header, buffer, recivedOnInterface);
    if(mip_header.sdu_type == ARP)
    {
      struct arpMsg arpMsg;
      u_int8_t* temp = malloc(sizeof(struct arpMsg));
      memcpy(&temp, buffer, sizeof(struct arpMsg));
      memcpy(&arpMsg, buffer, sizeof(struct arpMsg));
       if(mip_header.dst_addr == 0xFF)
       {
         if(arpMsg.type == 0x00 && arpMsg.mip_address == myMip)
         {
           addArpEntry(cache, mip_header.src_addr, ethernet_header.src_addr, *recivedOnInterface);
           sendArpResponse(socket_fd, interfaces, cache, mip_header.src_addr);
         }
       }

       //response
       else if(mip_header.dst_addr == myMip)
       {
         //response to my arp broadcast.
         if(arpMsg.type == 0x01)
         {
            addArpEntry(cache, mip_header.src_addr, ethernet_header.src_addr, *recivedOnInterface);
            printf("adding %d to cache \n", mip_header.src_addr);
         }
       }
    }

    else if(mip_header.sdu_type == PING)
    {
      applicationMsg msg;
      memset(&msg, 0, sizeof(struct applicationMsg));
      memcpy(&msg.address, &mip_header.src_addr, sizeof(u_int8_t));
      memcpy(msg.payload, buffer, strlen(buffer));
      sendApplicationMsg(activeApplication, msg.address, msg.payload);
    }

    free(buffer);
  }

void handleApplicationPacket(int activeApplication, int socket_fd, list* interfaces, list* cache)
{
  applicationMsg msg = readApplicationMsg(activeApplication);

  char* payload = malloc(1024);
  memset(payload, 0, 1024);

  if(msg.address == myMip)
  {
    write(activeApplication, msg.payload, strlen(msg.payload));
  }

  else if(msg.address != myMip)
  {
    arpEntry* entry = getCacheEntry(cache, msg.address);
    if(entry == NULL)
    {
      sendArpBroadcast(socket_fd, interfaces, msg.address);
      return;
    }

    mip_header mip_header;
    memset(&mip_header, 0, sizeof(struct mip_header));
    mip_header.src_addr = myMip;
    mip_header.dst_addr = msg.address;
    mip_header.sdu_type = 0x02;
    mip_header.ttl = 1;
    mip_header.sdu_length = strlen(payload);

    interface* interfaceToUse = getInterface(interfaces, entry -> via_interface);
    struct sockaddr_ll temp;
    memcpy(&temp, &interfaceToUse -> sock_addr, sizeof(struct sockaddr_ll));
    sendRawPacket(socket_fd, &temp, mip_header, msg.payload, strlen(payload), entry -> mac_address);
  }
}

int main(int argc, char* argv[])
{
      int debug_flag = false;

      if(argc == 2 && strcmp(argv[1], "-h"));
      {
          printf("Please specify <Pathname to domainsocket> <MIP address>\n");
          printf("You can optionally pass a -d (debug) flag");
          exit(0);
      }

      char* domainPath = argv[1];
      myMip = atoi(argv[2]);


    list* interfaces = createLinkedList(sizeof(interface));
    findInterfaces(interfaces);
    list* arpCache = createLinkedList(sizeof(arpEntry));
    int socket_fd, socket_unix;

    socket_unix = createDomainServerSocket(domainPath);
    socket_fd = createRawSocket();


    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
       perror("epoll_create");
       return -1;
     }

    struct epoll_event events[10];
    int amountOfEntries;
    addEpollEntry(socket_unix, epoll_fd);
    addEpollEntry(socket_fd, epoll_fd);
    int activeApplication = -1;

    while(true)
    {
      amountOfEntries = epoll_wait(epoll_fd, events, 10, -1);
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
               handleApplicationPacket(activeApplication, socket_fd, interfaces, arpCache);
             }

             else if(events[i].data.fd == socket_fd)
             {
               handleRawPacket(socket_fd, activeApplication, interfaces, arpCache);
             }
         }
       }
    }
  }
