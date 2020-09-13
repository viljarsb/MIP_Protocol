
#include "mip_deamon.h"

u_int8_t myMip;
int debug = 1;
list* interfaces;
list* arpCache;

void sendArpResponse(int socket_fd, u_int8_t dst_mip)
{
  arpEntry* entry = getCacheEntry(arpCache, dst_mip);

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
  if(debug == 1)
  {
    printf("\n\nSENDING ARP-RESPONSE -- SRC MIP: %d -- DST MIP: %d\n", mip_header.src_addr, mip_header.dst_addr);
  }
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
    if(debug == 1)
    {
      printf("\n\nSENDING ARP-BROADCAST -- SRC MIP: %d -- DST MIP: %d\n", myMip, lookup);
    }
    sendRawPacket(socket_fd, &temp, mip_header, buffer, sizeof(struct arpMsg), dst_addr);
    tempInterface = tempInterface -> next;
  }
  free(buffer);
}

void sendPing(int socket_fd, applicationMsg msg, list* interfaces, list* arpCache)
{
  arpEntry* entry = getCacheEntry(arpCache, msg.address);
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
  mip_header.sdu_length = strlen(msg.payload);


  interface* interfaceToUse = getInterface(interfaces, entry -> via_interface);
  struct sockaddr_ll temp;
  memcpy(&temp, &interfaceToUse -> sock_addr, sizeof(struct sockaddr_ll));
  if(debug == 1)
  {
    printf("\n\nSENDING PING -- MIP SRC: %d -- MIP DST: %d\n", mip_header.src_addr, mip_header.dst_addr);
  }
  sendRawPacket(socket_fd, &temp, mip_header, msg.payload, strlen(msg.payload), entry -> mac_address);
}

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
      u_int8_t* temp = malloc(sizeof(struct arpMsg));
      memcpy(&temp, buffer, sizeof(struct arpMsg));
      memcpy(&arpMsg, buffer, sizeof(struct arpMsg));
       if(mip_header.dst_addr == 0xFF)
       {
         if(arpMsg.type == 0x00 && arpMsg.mip_address == myMip)
         {
           if(debug == 1)
           {
             printf("RECEIVED ARP-REQUEST -- MIP SRC: %d -- MIP DST: %d\n", mip_header.src_addr, mip_header.dst_addr);
           }
           addArpEntry(arpCache, mip_header.src_addr, ethernet_header.src_addr, *recivedOnInterface);
           sendArpResponse(socket_fd, mip_header.src_addr);
         }
       }

       //response
       else if(mip_header.dst_addr == myMip)
       {
         //response to my arp broadcast.
         if(arpMsg.type == 0x01)
         {
           if(debug == 1)
           {
             printf("RECEIVED ARP-RESPONSE -- SRC MIP: %d -- DST MIP: %d \n", mip_header.src_addr, mip_header.dst_addr);
           }
            addArpEntry(arpCache, mip_header.src_addr, ethernet_header.src_addr, *recivedOnInterface);
         }
       }
    }

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

void handleApplicationPacket(int activeApplication, int socket_fd)
{
  applicationMsg msg = readApplicationMsg(activeApplication);
  //char* payload = malloc(1024);
  //emset(payload, 0, 1024);

  if(msg.address == myMip)
  {
    sendApplicationMsg(activeApplication, msg.address, msg.payload);
  }

  else if(msg.address != myMip)
  {
    sendPing(socket_fd, msg, interfaces, arpCache);
  }
}

int main(int argc, char* argv[])
{
    int debug_flag = false;

    char* domainPath = argv[1];
    myMip = atoi(argv[2]);

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
