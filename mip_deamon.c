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
#include "msgQ.h"
#include "epollFunctions.h"
#include "routing.h"

const u_int8_t REQUEST[3] = REQ;
const u_int8_t RESPONSE[3] = RSP;
const u_int8_t UPDATE[3] = UPD;
const u_int8_t HELLO[3] = HEL;


u_int8_t MY_MIP_ADDRESS;
bool debug = false;
list* interfaces;
list* arpCache;
msgQ* waitingQ;
msgQ* arpQ;
struct unsent
{
  mip_header* mip_header;
  char* buffer;
};
typedef struct unsent unsent;


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


/*
    The funtion constructs a mip-header with a ping-sdu and sends it over to
    another funciton along with the instruction of what interface to send on
    (according to the cache) and the destination mac address (according to cache).

    If the destination mip is not cached, the functions calls the send arp broadcast
    funtion to first obtain a correct mac to send to.

    @param  a socket-descriptor to send the packet on, the ping msg, a list of interfaes and an arp-cache.
*/
void sendApplicationData(int socket_fd, mip_header* mip_header, char* buffer, u_int8_t mipDst)
{
  arpEntry* entry = getCacheEntry(arpCache, mipDst);
  if(entry == NULL)
  {
    struct arpWaitEntry* arpWaitEntry = malloc(sizeof(struct arpWaitEntry));
    arpWaitEntry -> dst = mipDst;

    //FIND ANOTHER WAY!!
    struct mip_header* header = malloc(sizeof(mip_header));
    memcpy(header, mip_header, sizeof(mip_header));
    char* buffer2 = malloc(strlen(buffer));
    memcpy(buffer2, buffer, strlen(buffer));

    arpWaitEntry -> mip_header = header;
    arpWaitEntry -> buffer = buffer2;
    push(arpQ, arpWaitEntry);
    sendArpBroadcast(socket_fd, interfaces, mipDst);
    return;
  }

  interface* interfaceToUse = getInterface(interfaces, entry -> via_interface);
  struct sockaddr_ll temp;
  memcpy(&temp, &interfaceToUse -> sock_addr, sizeof(struct sockaddr_ll));
  if(debug)
    printf("\n\nSENDING PING -- MIP SRC: %d -- MIP DST: %d\n", mip_header -> src_addr, mip_header -> dst_addr);
  sendRawPacket(socket_fd, &temp, mip_header, buffer, strlen(buffer), entry -> mac_address);
}

/*
    Function read and inspects incoming raw packets.
    The function may send out arp responses to broadcasts.
    The function may also cache addresses based on incoming responses.
    The function can also forward msgs to the current active application in case it reads a ping.

    @Param a raw socket and the current active application socket.
*/
void handleRawPacket(int socket_fd, int pingApplication, int routingApplication)
{
    ethernet_header* ethernet_header = malloc(sizeof(ethernet_header));
    mip_header* mip_header = malloc(sizeof(mip_header));

    memset(ethernet_header, 0, sizeof(struct ethernet_header));
    memset(mip_header, 0, sizeof(struct mip_header));
    char* buffer = malloc(1024);
    memset(buffer, 0, 1024);
    int* recivedOnInterface = malloc(sizeof(int));

    readRawPacket(socket_fd, ethernet_header, mip_header, buffer, recivedOnInterface);

    if(mip_header -> sdu_type == ARP)
    {
      struct arpMsg arpMsg;
      memcpy(&arpMsg, buffer, sizeof(struct arpMsg));

      //request
       if(mip_header -> dst_addr == 0xFF && arpMsg.type == ARP_BROADCAST)
       {
         if(arpMsg.type == ARP_BROADCAST && arpMsg.mip_address == MY_MIP_ADDRESS)
         {
           if(getCacheEntry(arpCache, mip_header -> src_addr) == NULL)
             addArpEntry(arpCache, mip_header -> src_addr, ethernet_header -> src_addr, *recivedOnInterface);

           else
             updateArpEntry(arpCache, mip_header -> src_addr, ethernet_header -> src_addr, *recivedOnInterface);

           if(debug)
           {
             printf("RECEIVED ARP-REQUEST -- MIP SRC: %d -- MIP DST: %d\n", mip_header -> src_addr, mip_header -> dst_addr);
             printArpCache(arpCache);
           }
           sendArpResponse(socket_fd, mip_header -> src_addr);
         }

         else
         {
           if(getCacheEntry(arpCache, mip_header -> src_addr) == NULL)
           {
             addArpEntry(arpCache, mip_header -> src_addr, ethernet_header -> src_addr, *recivedOnInterface);
             if(debug)
              printf("INTERCEPTED A ARP-BROADCAST MEANT FOR ANOTHER HOST\n ADDING TO CACHE ANYWAY");
           }
           else
           {
             updateArpEntry(arpCache, mip_header -> src_addr, ethernet_header -> src_addr, *recivedOnInterface);
           }
         }
       }

       //response
       else if(mip_header -> dst_addr == MY_MIP_ADDRESS && arpMsg.type == ARP_RESPONSE)
       {
        addArpEntry(arpCache, mip_header -> src_addr, ethernet_header -> src_addr, *recivedOnInterface);

        if(debug)
        {
          printf("RECEIVED ARP-RESPONSE -- SRC MIP: %d -- DST MIP: %d \n", mip_header -> src_addr, mip_header -> dst_addr);
          printArpCache(arpCache);
        }

        if(arpQ -> amountOfEntries > 0)
        {
          struct arpWaitEntry* arpWaitEntry = pop(arpQ);
          sendApplicationData(socket_fd, arpWaitEntry -> mip_header, arpWaitEntry -> buffer, arpWaitEntry -> dst);
        /*      u_int8_t addr;
              applicationMsg* appMsg;
              memcpy(&addr, &unsent, sizeof(u_int8_t));
              memcpy(&appMsg, &unsent + 1, sizeof(struct applicationMsg));
              sendApplicationData(socket_fd, appMsg, addr);*/
        }
     }
  }

    //ping
    else if(mip_header -> sdu_type == PING)
    {
      if(mip_header -> dst_addr == MY_MIP_ADDRESS)
      {
        applicationMsg msg;
        memset(&msg, 0, sizeof(struct applicationMsg));
        memcpy(&msg.address, &mip_header -> src_addr, sizeof(u_int8_t));
        memcpy(msg.payload, buffer, sizeof(struct applicationMsg));

        if(debug)
        {
          printf("RECEIVED PING -- MIP SRC: %d -- MIP DST: %d\n", mip_header -> src_addr, mip_header -> dst_addr);
        }

        if(pingApplication != -1)
          sendApplicationMsg(pingApplication, msg.address, msg.payload, strlen(msg.payload));

        else
          if(debug)
            printf("CAN NOT SEND DATA TO PING APPLICATION -- NO SUCH APPLICATION IS CURRENTLY RUNNING\n");
      }

      else if(mip_header -> dst_addr != MY_MIP_ADDRESS)
      {
        if(debug)
          printf("RECEIVED PING FOR ANOHTER HOST -- FORWARDING TO %u FOR %u\n", mip_header -> dst_addr, mip_header -> src_addr);
        unsent* unsent = malloc(sizeof(unsent));
        unsent -> mip_header = mip_header;
        unsent -> buffer = buffer;
        push(waitingQ, unsent);
        SendForwardingRequest(routingApplication, mip_header -> dst_addr);
      }
    }

    //routing
    else if(mip_header -> sdu_type == ROUTING)
    {
      if(debug)
        printf("RECIEVIED ROUTING-SDU -- MIP SRC: %u -- MIP DEST: %u\n", mip_header -> src_addr, mip_header -> dst_addr);

      applicationMsg msg;
      memset(&msg, 0, sizeof(struct applicationMsg));
      memcpy(&msg.address, &mip_header -> src_addr, sizeof(u_int8_t));
      memcpy(msg.payload, buffer, sizeof(struct applicationMsg));

      if(routingApplication != -1)
        sendApplicationMsg(routingApplication, msg.address, msg.payload, sizeof(msg) - 1);

      else
        if(debug)
          printf("CAN NOT SEND DATA TO ROUTINGDEAMON -- NO ROUTINGDEAMON RUNNING\n");
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
void handleApplicationPacket(int activeApplication, int routingApplication, int socket_fd)
{
  applicationMsg* msg = calloc(1, sizeof(applicationMsg));
  int rc = readApplicationMsg(activeApplication, msg);

  if(msg -> address == MY_MIP_ADDRESS)
  {
    if(debug)
      printf("AN APPLICATION ON THIS HOST ADDRESS A PACKET TO IT SELF -- SENDING THE MSG IN RETURN\n");

    sendApplicationMsg(activeApplication, msg -> address, msg -> payload, strlen(msg -> payload));
  }

  else if(msg -> address != MY_MIP_ADDRESS)
  {
    SendForwardingRequest(routingApplication, msg -> address);
    mip_header* mip_header = malloc(sizeof(mip_header));
    mip_header -> src_addr = MY_MIP_ADDRESS;
    mip_header -> dst_addr = msg -> address;
    mip_header -> sdu_type = PING;
    mip_header -> ttl = msg -> TTL;
    mip_header -> sdu_length = strlen(msg -> payload);
    struct unsent* unsent = malloc(sizeof(struct unsent));
    unsent -> mip_header = mip_header;
    unsent -> buffer = msg -> payload;
    push(waitingQ, unsent);
  }
}

void sendRoutingBroadcast(int socket_fd, applicationMsg* msg, int len)
{
  mip_header mip_header;
  mip_header.src_addr = MY_MIP_ADDRESS;
  mip_header.dst_addr = 0xFF;
  mip_header.sdu_type = ROUTING;
  mip_header.ttl = msg -> TTL;
  mip_header.sdu_length = len;

  uint8_t dst_addr[ETH_ALEN] = BROADCAST_MAC_ADDR;

  node* tempInterface = interfaces -> head;
  struct sockaddr_ll temp;

  while(tempInterface != NULL)
  {
    interface* currentInterface = (struct interface*) tempInterface -> data;
    memcpy(&temp, &currentInterface -> sock_addr, sizeof(struct sockaddr_ll));
    if(debug)
      printf("\n\nSENDING ROUTING-BROADCAST -- SRC MIP: %d -- DST MIP: %d\n", MY_MIP_ADDRESS, 0xFF);
    sendRawPacket(socket_fd, &temp, &mip_header, msg -> payload, mip_header.sdu_length, dst_addr);
    tempInterface = tempInterface -> next;
  }
}

void handleRoutingPacket(int socket_fd, int routingApplication)
{
  applicationMsg* appMsg = calloc(1, sizeof(applicationMsg));
  int rc = readApplicationMsg(routingApplication, appMsg);

  if(memcmp(RESPONSE, appMsg -> payload, sizeof(RESPONSE)) == 0)
  {
    routingQuery query;
    memcpy(&query, appMsg -> payload, sizeof(routingQuery));

    if(query.mip == 255)
    {
        if(debug)
          printf("NO ROUTE FOUND FOR DESTINATION: %u -- DISCARDRING PACKET\n", appMsg -> address);
      }

    else
    {
        unsent* unsent = pop(waitingQ);
        if(debug)
          printf("ROUTE FOUND FOR DESTINATION: %u -- ROUTING VIA: %u\n", unsent -> mip_header -> dst_addr, query.mip);
        sendApplicationData(socket_fd, unsent -> mip_header, unsent -> buffer, query.mip);
      }
  }

  else if(memcmp(HELLO, appMsg -> payload, sizeof(HELLO)) == 0 || memcmp(UPDATE, appMsg -> payload, sizeof(UPDATE)) == 0)
  {
    sendRoutingBroadcast(socket_fd, appMsg, rc - (sizeof(u_int8_t) * 2));
  }
}

void SendForwardingRequest(int routingSocket, u_int8_t mip)
{
  struct routingQuery query;
  char* buffer = calloc(1, sizeof(routingQuery));

  memcpy(query.type, REQUEST, sizeof(REQUEST));
  query.mip = mip;

  memcpy(buffer, &query, sizeof(routingQuery));

  sendApplicationMsg(routingSocket, MY_MIP_ADDRESS, buffer, sizeof(routingQuery));
}

//Very simple signal handler.
void handle_sigint(int sig)
{
    if(debug)
      printf("MIP_DEAMON FORCE-QUIT\n");
    freeInterfaces(interfaces);
    freeListMemory(arpCache);
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
  char* domainPath;

    if(argc == 4 && strcmp(argv[1], "-d") == 0)
    {
      debug = true;
      domainPath = argv[2];
      MY_MIP_ADDRESS = atoi(argv[3]);
    }

    else if(argc == 2 && strcmp(argv[1], "-h") == 0)
    {
      printf("Run program with -d (optional debugmode) <domain path> <mip address>\n");
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
    waitingQ = createQ();
    arpQ = createQ();
    int socket_fd, socket_application, socket_routing;

    socket_application = createDomainServerSocket(domainPath);
    socket_fd = createRawSocket();

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
       perror("epoll_create");
       exit(EXIT_FAILURE);
     }

    struct epoll_event events[10];
    int amountOfEntries;
    addEpollEntry(socket_application, epoll_fd);
    addEpollEntry(socket_fd, epoll_fd);

    int pingApplication = -1;
    int routingApplication = -1;
    int tempApplication = -1;

    while(true)
    {
      amountOfEntries = epoll_wait(epoll_fd, events, 10, -1);
      signal(SIGINT, handle_sigint);
       for (int i = 0; i < amountOfEntries; i++)
       {
         if(events[i].data.fd == socket_application)
         {
           tempApplication = accept(socket_application, NULL, NULL);
           u_int8_t sdu_type = 0x00;
           read(tempApplication, &sdu_type, sizeof(u_int8_t));

           if(sdu_type == PING)
           {
             if(debug)
              printf("\n\nA PING APPLICATION CONNECTED\n");
             pingApplication = tempApplication;
             tempApplication = -1;
             addEpollEntry(pingApplication, epoll_fd);
           }

           if(sdu_type == ROUTING)
           {
             if(debug)
              printf("\n\nA ROUTING_DEAMON APPLICATION CONNECTED\n");
             routingApplication = tempApplication;
             tempApplication = -1;
             addEpollEntry(routingApplication, epoll_fd);
             write(routingApplication, &MY_MIP_ADDRESS, sizeof(u_int8_t));
           }
         }

         //closing
         else if(events[i].events & EPOLLHUP)
         {
           if(events[i].data.fd == routingApplication)
           {
             if(debug)
              printf("\n\nA ROUTING APPLICATION DISCONNECTED\n");
             removeEpollEntry(routingApplication, epoll_fd);
             close(routingApplication);
             routingApplication = -1;
           }

           else if(events[i].data.fd == pingApplication)
           {
             removeEpollEntry(pingApplication, epoll_fd);
             close(pingApplication);
             pingApplication = -1;
             if(debug)
              printf("\n\nA PING APPLICATION DISCONNECTED\n");
           }

         }

         //data
         else if (events[i].events & EPOLLIN)
         {
             if(events[i].data.fd == pingApplication)
             {
               handleApplicationPacket(pingApplication, routingApplication, socket_fd);
             }

             else if(events[i].data.fd == routingApplication)
             {
               handleRoutingPacket(socket_fd, routingApplication);
             }

             else if(events[i].data.fd == socket_fd)
             {
               handleRawPacket(socket_fd, pingApplication, routingApplication);
             }
         }
       }
    }
  }
