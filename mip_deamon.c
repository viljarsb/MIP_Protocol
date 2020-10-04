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


void handleRawPacket(int socket_fd, int pingApplication, int routingApplication)
{
    ethernet_header* ethernet_header = calloc(1, sizeof(struct ethernet_header));
    mip_header* mip_header = calloc(1, sizeof(struct mip_header));
    char* buffer = calloc(1, 1024);
    int* recivedOnInterface = calloc(1, sizeof(int));

    readRawPacket(socket_fd, ethernet_header, mip_header, buffer, recivedOnInterface);

    if(mip_header -> sdu_type == ARP)
    {
      struct arpMsg arpMsg;
      memcpy(&arpMsg, buffer, sizeof(struct arpMsg));

      //request
       if(mip_header -> dst_addr == 0xFF && arpMsg.type == ARP_BROADCAST)
       {
         if(arpMsg.mip_address == MY_MIP_ADDRESS)
         {
           if(getCacheEntry(arpCache, mip_header -> src_addr) == NULL)
             addArpEntry(arpCache, mip_header -> src_addr, ethernet_header -> src_addr, *recivedOnInterface);

           else
             updateArpEntry(arpCache, mip_header -> src_addr, ethernet_header -> src_addr, *recivedOnInterface);

           if(debug)
           {
             printf("RECEIVED ARP-REQUEST -- MIP SRC: %d -- MIP DST: %d\n\n", mip_header -> src_addr, mip_header -> dst_addr);
             printArpCache(arpCache);
           }
           sendArpResponse(socket_fd, mip_header -> src_addr);
         }
       }

       //response
       else if(mip_header -> dst_addr == MY_MIP_ADDRESS && arpMsg.type == ARP_RESPONSE)
       {
        addArpEntry(arpCache, mip_header -> src_addr, ethernet_header -> src_addr, *recivedOnInterface);

        if(debug)
        {
          printf("RECEIVED ARP-RESPONSE -- SRC MIP: %d -- DST MIP: %d\n", mip_header -> src_addr, mip_header -> dst_addr);
          printArpCache(arpCache);
        }

        if(arpQ -> amountOfEntries > 0)
        {
          struct arpWaitEntry* arpWaitEntry = pop(arpQ);
          sendApplicationData(socket_fd, arpWaitEntry -> mip_header, arpWaitEntry -> buffer, arpWaitEntry -> dst);
          free(arpWaitEntry);
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
        memcpy(msg.payload, buffer, mip_header -> sdu_length);

        if(debug)
          printf("RECEIVED PING -- MIP SRC: %d -- MIP DST: %d\n\n", mip_header -> src_addr, mip_header -> dst_addr);

        if(pingApplication != -1)
          sendApplicationMsg(pingApplication, msg.address, msg.payload, strlen(msg.payload));

        else
          if(debug)
            printf("CAN NOT SEND DATA TO PING APPLICATION -- NO SUCH APPLICATION IS CURRENTLY RUNNING\n\n");
      }

      else if(mip_header -> dst_addr != MY_MIP_ADDRESS)
      {

        if(mip_header -> ttl - 1 <= 0)
        {
          if(debug)
            printf("RECEIVED PING FOR ANOHTER HOST -- TTL HAS REACHED 0 -- DISCARDING PACKET WITH DESTINATION: %u\n\n", mip_header -> dst_addr);
          return;
        }

        if(debug)
          printf("RECEIVED PING FOR ANOHTER HOST -- TRYING TO FORWARDING TO %u FOR %u\n\n", mip_header -> dst_addr, mip_header -> src_addr);

        unsent* unsent = malloc(sizeof(struct unsent));
        unsent -> mip_header = malloc(sizeof(struct mip_header));
        unsent -> buffer = malloc(mip_header -> sdu_length);
        memcpy(unsent -> mip_header, mip_header, sizeof(struct mip_header));
        memcpy(unsent -> buffer, buffer, mip_header -> sdu_length);
        push(waitingQ, unsent);
        SendForwardingRequest(routingApplication, unsent -> mip_header -> dst_addr);
      }
    }

    //routing
    else if(mip_header -> sdu_type == ROUTING)
    {
      if(debug)
        printf("RECIEVIED ROUTING-SDU -- MIP SRC: %u -- MIP DEST: %u\n\n", mip_header -> src_addr, mip_header -> dst_addr);

      applicationMsg msg;
      memset(&msg, 0, sizeof(struct applicationMsg));
      memcpy(&msg.address, &mip_header -> src_addr, sizeof(u_int8_t));
      memcpy(msg.payload, buffer, mip_header -> sdu_length);

      if(routingApplication != -1)
        sendApplicationMsg(routingApplication, msg.address, msg.payload, mip_header -> sdu_length);

      else
        if(debug)
          printf("CAN NOT SEND DATA TO ROUTINGDEAMON -- NO ROUTINGDEAMON RUNNING\n\n");
    }

    free(ethernet_header);
    free(recivedOnInterface);
    free(mip_header);
    free(buffer);
  }

void handleApplicationPacket(int activeApplication, int routingApplication, int socket_fd)
{
  applicationMsg* msg = calloc(1, sizeof(applicationMsg));
  int rc = readApplicationMsg(activeApplication, msg);

  if(debug)
    printf("RECEIVED %d BYTES FROM APPLICATION\n", rc);

  if(msg -> address == MY_MIP_ADDRESS)
  {
    if(debug)
      printf("A APPLICATION ON THIS HOST ADDRESS A PACKET TO IT SELF -- SENDING THE MSG IN RETURN\n\n");

    sendApplicationMsg(activeApplication, msg -> address, msg -> payload, strlen(msg -> payload));
  }

  else if(msg -> address != MY_MIP_ADDRESS)
  {
    SendForwardingRequest(routingApplication, msg -> address);
    struct unsent* unsent = malloc(sizeof(struct unsent));
    unsent -> mip_header = calloc(1, sizeof(struct mip_header));
    unsent -> mip_header -> src_addr = MY_MIP_ADDRESS;
    unsent -> mip_header -> dst_addr = msg -> address;
    unsent -> mip_header -> sdu_type = PING;
    unsent -> mip_header -> ttl = msg -> TTL;
    unsent -> mip_header -> sdu_length = strlen(msg -> payload);
    unsent -> buffer = calloc(1, unsent -> mip_header -> sdu_length);
    memcpy(unsent -> buffer, msg -> payload, unsent -> mip_header -> sdu_length);
    push(waitingQ, unsent);
  }

  free(msg);
}

void sendRoutingBroadcast(int socket_fd, char* payload, int len)
{
  if(debug)
    printf("SENDING ROUTING-SDU -- MIP SRC: %u -- MIP DEST: %u\n", MY_MIP_ADDRESS, 0xFF);
  mip_header* mip_header = calloc(1, sizeof(struct mip_header));
  char* buffer = calloc(1, len);
  memcpy(buffer, payload, len);
  free(payload);

  mip_header -> src_addr = MY_MIP_ADDRESS;
  mip_header -> dst_addr = 0xFF;
  mip_header -> sdu_type = ROUTING;
  mip_header -> ttl = 1;
  mip_header -> sdu_length = len;

  sendApplicationData(socket_fd, mip_header, buffer, 0xFF);
}

void handleRoutingPacket(int socket_fd, int routingApplication)
{
  applicationMsg* appMsg = calloc(1, sizeof(applicationMsg));
  int rc = readApplicationMsg(routingApplication, appMsg);

  if(memcmp(RESPONSE, appMsg -> payload, sizeof(RESPONSE)) == 0)
  {
    routingQuery query;
    memcpy(&query, &appMsg -> payload, sizeof(routingQuery));

    if(query.mip == 255)
    {
      if(debug)
        printf("NO ROUTE FOUND FOR DESTINATION: %u -- DISCARDRING PACKET\n", appMsg -> address);
      free(pop(waitingQ));
      return;
    }

    else
    {
        unsent* unsent = pop(waitingQ);
        if(debug)
        {
          printf("ROUTE FOUND FOR DESTINATION: %u -- ROUTING VIA: %u\n", unsent -> mip_header -> dst_addr, query.mip);
          printf("SENDING PING-SDU -- MIP SRC: %u -- MIP DST: %u\n", unsent -> mip_header -> src_addr, unsent -> mip_header -> dst_addr);
        }

        mip_header* mip_header = calloc(1, sizeof(struct mip_header));
        char* buffer = calloc(1, unsent -> mip_header -> sdu_length);
        memcpy(mip_header, unsent -> mip_header, sizeof(struct mip_header));
        memcpy(buffer, unsent -> buffer, mip_header -> sdu_length);

        free(unsent -> mip_header);
        free(unsent -> buffer);
        free(unsent);
        sendApplicationData(socket_fd, mip_header, buffer, query.mip);
      }
  }

  else if(memcmp(HELLO, appMsg -> payload, sizeof(HELLO)) == 0 || memcmp(UPDATE, appMsg -> payload, sizeof(UPDATE)) == 0)
  {
    char* buffer = malloc(rc - (sizeof(u_int8_t) * 2));
    memcpy(buffer, appMsg -> payload, rc - (sizeof(u_int8_t) * 2));
    sendRoutingBroadcast(socket_fd, buffer, rc - (sizeof(u_int8_t) * 2));
  }

  free(appMsg);
}

void SendForwardingRequest(int routingSocket, u_int8_t mip)
{
  struct routingQuery query;
  char* buffer = calloc(1, sizeof(routingQuery));
  memcpy(query.type, REQUEST, sizeof(REQUEST));
  query.mip = mip;
  memcpy(buffer, &query, sizeof(routingQuery));
  sendApplicationMsg(routingSocket, MY_MIP_ADDRESS, buffer, sizeof(routingQuery));
  free(buffer);
}

void handle_sigint(int sig)
{
    if(debug)
      printf("MIP_DEAMON FORCE-QUIT\n");
    freeInterfaces(interfaces);
    freeListMemory(arpCache);
    free(arpQ);
    free(waitingQ);

    //TELL OTHER NODES IM GONE!
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
              printf("A PING APPLICATION CONNECTED\n\n");
             pingApplication = tempApplication;
             tempApplication = -1;
             addEpollEntry(pingApplication, epoll_fd);
           }

           if(sdu_type == ROUTING)
           {
             if(debug)
              printf("A ROUTING_DEAMON APPLICATION CONNECTED\n\n");
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
