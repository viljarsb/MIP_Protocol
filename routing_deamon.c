#include <stdio.h>
#include "unistd.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <string.h>
#include "routing_deamon.h"
#include "epollFunctions.h"
#include "socketFunctions.h"
#include "linkedList.h"
#include "routing.h"
#include "protocol.h"
#include "applicationFunctions.h"

u_int8_t REQUEST[3] = REQ;
u_int8_t RESPONSE[3] = RSP;
u_int8_t UPDATE[3] = UPD;
u_int8_t HELLO[3] = HEL;
u_int8_t MY_MIP_ADDRESS;
int routingSocket;
routingEntry* routingTable[255];

void sendRoutingMsg(u_int8_t mip_one, u_int8_t ttl, u_int8_t type[3], u_int8_t mip_two)
{
  char* buffer = calloc(1, sizeof(struct routingMsgIntra) + sizeof(u_int8_t));
  struct routingMsgIntra msg;
  memcpy(msg.type, type, sizeof(u_int8_t) * 3);
  msg.mip_other = mip_two;
  memcpy(buffer, &msg, sizeof(struct routingMsgIntra));
  sendApplicationMsg(routingSocket, MY_MIP_ADDRESS, buffer, sizeof(struct routingMsgIntra));
}

void sendResponse(int destination, u_int8_t next_hop)
{
  printf("SENDING ROUTING-RESPONSE FOR MIP: %u\n\n", destination);
  sendRoutingMsg(MY_MIP_ADDRESS, 0, RESPONSE, next_hop);
}

void sendHello()
{
  printf("SENDING HELLO-MSG TO NEIGHBOURS\n\n");
  sendRoutingMsg(MY_MIP_ADDRESS, 1, HELLO, 255);
}
/*
void sendUpdate()
{
  char* buffer = malloc(sizeof(struct routingMsgIntra));
  struct routingMsgIntra msg;
  msg.mip_local = MY_MIP_ADDRESS;
  msg.ttl = 1;
  memcpy(msg.type, UPDATE, sizeof(u_int8_t) * 3);
  msg.mip_other = 0;
  memcpy(buffer, &msg, sizeof(struct routingMsgIntra));
  int counter = sizeof(msg);
  for(int i = 0; i < 255; i++)
  {
    if(routingTable[i] != NULL)
    {
      printf("WTF\n");
      buffer = realloc(buffer, counter + sizeof(struct routingEntry));
      memcpy(buffer + counter, routingTable[i], sizeof(struct routingEntry));
      counter = counter + sizeof(routingEntry);
    }
  }
  printf("SENDING UPDATE-MSG TO NEIGHBOURS\n");
  sendApplicationMsg(routingSocket, MY_MIP_ADDRESS, buffer);
}*/

routingEntry* findEntry(u_int8_t mip)
{
  return routingTable[mip];
}

void addToRoutingTable(u_int8_t mip, u_int8_t cost, u_int8_t next)
{
  routingEntry entry;
  entry.mip_address = mip;
  entry.cost = cost;
  entry.next_hop = next;
  routingTable[mip] = &entry;
  printf("ADDING TO ROUTINGTABLE -- MIP: %d -- COST: %u -- NEXT HOP %u\n\n", entry.mip_address, entry.cost, entry.next_hop);
}

int findNextHop(u_int8_t mip)
{
  routingEntry* entry = findEntry(mip);
  if(entry == NULL)
    return 255;

  return entry -> next_hop;
}

void handleIncomingMsg()
{
  //char* buffer = malloc(1024);
  applicationMsg applicationMsg = readApplicationMsg(routingSocket);
  struct routingMsgIntra msg;
  memcpy(msg.type, applicationMsg.payload, 3);


  if(memcmp(msg.type, REQUEST, sizeof(u_int8_t) * 3) == 0)
  {
      printf("RECIEVIED ROUTING-REQUEST\n");
      msg.mip_other = applicationMsg.payload[3];
      u_int8_t next_hop = findNextHop(msg.mip_other);
      printf("%u %u \n", msg.mip_other, next_hop);
      sendResponse(msg.mip_other, next_hop);
    }

    else if(memcmp(msg.type, HELLO, sizeof(u_int8_t) * 3) == 0)
    {
      routingEntry entry;
      entry.mip_address = applicationMsg.address;
      entry.cost = 1;
      entry.next_hop = applicationMsg.address;
      routingTable[entry.mip_address] = &entry;
      printf("RECIEVIED HELLO-BROADCAST -- ADDING %u TO ROUTINGTABLE WITH NEXT HOP %u\n", entry.mip_address, entry.next_hop);
  //    sendUpdate();
    }

/*
    else if(memcmp(msg.type, UPDATE, sizeof(u_int8_t) * 3) == 0)
    {
        printf("RECIEVIED ROUTING-UPDATE -- UPDATING ROUTINGTABLE\n");
        int i = sizeof(struct routingMsgIntra);
        struct routingEntry entry;
        while(i < rc)
        {
          memcpy(&entry, buffer + i, sizeof(struct routingEntry));
          printf("entry mip %u entry cost %u entryhop %u\n", entry
          .mip_address, entry.cost, entry.next_hop);
          i = i + sizeof(struct routingEntry);


          if(routingTable[entry.mip_address] == NULL)
          {
            addToRoutingTable(entry.mip_address, entry.cost + 1, msg.mip_local);
          }

          else if(routingTable[entry.mip_address] != NULL)
          {
            if(entry.cost + 1 < routingTable[entry.mip_address] -> cost)
            {
              routingTable[entry.mip_address] -> cost = entry.cost + 1;
              routingTable[entry.mip_address] -> next_hop = msg.mip_local;
              printf("UPDATED\n");
            }
          }
          printf("JADA\n");
        }
    }*/
}

int main(int argc, char* argv[])
{
//  int routingSocket;
  char* domainPath;

  if(argc == 2)
  {
    domainPath = argv[1];
  }

    routingSocket = createDomainClientSocket(domainPath);
    u_int8_t temp = ROUTING;
    write(routingSocket, &temp, sizeof(u_int8_t));
    read(routingSocket, &MY_MIP_ADDRESS, sizeof(u_int8_t));
    addToRoutingTable(MY_MIP_ADDRESS, 0, MY_MIP_ADDRESS);
    sendHello(routingSocket);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
       perror("epoll_create");
       return -1;
     }

    struct epoll_event events[1];
    int amountOfEntries;
    addEpollEntry(routingSocket, epoll_fd);

    //Loop forever.
    while(1)
    {
      //block until the file descriptor/socket has data to be processed.
      amountOfEntries = epoll_wait(epoll_fd, events, 1, -1);
       for (int i = 0; i < amountOfEntries; i++)
       {
         if (events[i].events & EPOLLIN)
         {
            if(events[i].data.fd == routingSocket)
           {
             handleIncomingMsg();
           }
         }
       }
    }



}
