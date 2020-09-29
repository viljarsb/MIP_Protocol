#include <stdio.h>
#include "unistd.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdbool.h>
#include "routing_deamon.h"
#include "epollFunctions.h"
#include "socketFunctions.h"
#include "linkedList.h"
#include "protocol.h"
#include "applicationFunctions.h"

routingEntry* routingTable[255];
u_int8_t REQUEST[3] = REQ;
u_int8_t RESPONSE[3] = RSP;
u_int8_t UPDATE[3] = UPD;
u_int8_t HELLO[3] = HEL;
u_int8_t MY_MIP_ADDRESS;
int routingSocket;

routingEntry* findEntry(u_int8_t mip)
{
  return routingTable[mip];
}

int findNextHop(u_int8_t mip)
{
  routingEntry* entry = findEntry(mip);
  if(entry == NULL)
    return 255;

  return entry -> next_hop;
}

void sendRoutingMsg(u_int8_t mip_one, char* buffer, int len)
{
  sendApplicationMsg(routingSocket, mip_one, buffer, len);
}

void sendResponse(int destination, u_int8_t next_hop)
{
  printf("SENDING ROUTING-RESPONSE FOR MIP: %u\n\n", destination);

  routingMsg msg;
  memcpy(msg.type, RESPONSE, sizeof(RESPONSE));
  msg.data = malloc(sizeof(u_int8_t));
  memcpy(&msg.data, &next_hop, sizeof(u_int8_t));
  char* buffer = calloc(1, sizeof(RESPONSE) + sizeof(u_int8_t));
  memcpy(buffer, &msg.type, sizeof(RESPONSE));
  memcpy(buffer + sizeof(RESPONSE), &msg.data, sizeof(u_int8_t));
  sendRoutingMsg(destination, buffer, sizeof(RESPONSE) + sizeof(u_int8_t));
  free(buffer);
}

void sendHello()
{
  printf("SENDING HELLO-MSG TO NEIGHBOURS\n\n");
  char* buffer = calloc(1, 3);
  routingMsg msg;
  memcpy(msg.type, HELLO, sizeof(HELLO));
  memcpy(buffer, &msg, sizeof(HELLO));
  sendRoutingMsg(MY_MIP_ADDRESS, buffer, sizeof(HELLO));
  free(buffer);
}

void sendUpdate()
{
  printf("SENDING UPDATE-MSG TO NEIGHBOURS\n\n");
  char* buffer = calloc(1, 3);
  routingMsg msg;
  memcpy(msg.type, UPDATE, sizeof(UPDATE));
  memcpy(buffer, &msg.type, sizeof(UPDATE));

  struct updateStructure updateStructure;
  updateStructure.amount = 0;
  updateStructure.data = malloc(0);

  for(int i = 0; i < 255; i++)
  {
    if(findEntry(i) != NULL)
    {
      updateStructure.amount = updateStructure.amount + 1;
      updateStructure.data = realloc(updateStructure.data, sizeof(routingEntry) * updateStructure.amount);
      memcpy(updateStructure.data + (updateStructure.amount-1) * sizeof(routingEntry), &*routingTable[i], sizeof(struct routingEntry));
    }
  }

  buffer = realloc(buffer, sizeof(u_int8_t) * 4 + (sizeof(routingEntry) * updateStructure.amount));
  memcpy(buffer + 3, &updateStructure.amount, sizeof(u_int8_t));
  memcpy(buffer + 4, updateStructure.data, sizeof(routingEntry) * updateStructure.amount);
  sendRoutingMsg(MY_MIP_ADDRESS, buffer, 4 + (updateStructure.amount * sizeof(routingEntry)));
}

void addToRoutingTable(u_int8_t mip, u_int8_t cost, u_int8_t next)
{
  if(findEntry(mip) == NULL)
  {
    printf("ADDED %u TO ROUTING-TABLE -- COST: %u -- NEXT HOP: %u\n\n", mip, cost, next);
    routingEntry* entry = malloc(sizeof(routingEntry));
    entry -> mip_address = mip;
    entry -> cost = cost;
    entry -> next_hop = next;
    routingTable[mip] = entry;
  }

  else
  {
    updateRoutingEntry(mip, cost, next);
  }
}

void updateRoutingEntry(u_int8_t mip, u_int8_t newCost, u_int8_t newNext)
{
  routingEntry* entry = findEntry(mip);
  entry -> cost = newCost;
  entry -> next_hop = newNext;
}

void handleIncomingMsg()
{
  applicationMsg* applicationMsg = calloc(1, sizeof(struct applicationMsg));
  int rc = readApplicationMsg(routingSocket, applicationMsg);
  routingMsg msg;
  memcpy(msg.type, applicationMsg -> payload, 3);
  msg.data = malloc(rc - 5);
  memcpy(msg.data, applicationMsg -> payload + 3, rc - 5);

  if(memcmp(msg.type, REQUEST, sizeof(REQUEST)) == 0)
  {
      printf("RECIEVIED ROUTING-REQUEST\n");
      u_int8_t addr;
      u_int8_t next_hop;
      memcpy(&addr, msg.data, sizeof(u_int8_t));
      next_hop = findNextHop(addr);
      sendResponse(addr, next_hop);
    }

  else if(memcmp(msg.type, HELLO, sizeof(HELLO)) == 0)
  {
    printf("RECIEVIED HELLO-BROADCAST -- ADDING TO ROUTINGTABLE\n");
    addToRoutingTable(applicationMsg -> address, 1, applicationMsg -> address);
    sendUpdate();
  }

  else if(memcmp(msg.type, UPDATE, sizeof(UPDATE)) == 0)
  {
        printf("RECIEVIED ROUTING-UPDATE -- UPDATING ROUTINGTABLE\n\n");
        bool changed = false;
        updateStructure updateStructure;
        memcpy(&updateStructure.amount, msg.data, sizeof(u_int8_t));
        updateStructure.data = malloc(updateStructure.amount * sizeof(routingEntry));
        memcpy(updateStructure.data, msg.data + 1, updateStructure.amount * sizeof(routingEntry));

        int currentPos = 1;
        int counter = 0;
        routingEntry entry;
        while(counter < updateStructure.amount)
        {
          memcpy(&entry, msg.data + currentPos, sizeof(struct routingEntry));
          currentPos = currentPos + sizeof(struct routingEntry);

          if(routingTable[entry.mip_address] == NULL)
          {
            addToRoutingTable(entry.mip_address, entry.cost + 1, applicationMsg -> address);
            changed = true;
          }

          else if(routingTable[entry.mip_address] != NULL)
          {
            if(entry.cost + 1 < findEntry(entry.mip_address) -> cost)
            {
              updateRoutingEntry(entry.mip_address, entry.cost + 1, applicationMsg -> address);
              changed = true;
              printf("UPDATED NEXT-HOP OF %u\n\n", entry.mip_address);
            }
          }
          counter = counter +1;
    }

    if(changed)
    {
      printf("UPDATED COMPLETE -- TABLE WAS ALTERED -- SENDING UPDATE TO NEIGHBOURS\n\n");
      sendUpdate();
    }

    else
      printf("UPDATED COMPLETE -- TABLE WAS NOT ALTERED\n\n");

      printf("CURRENT ROUTINGTABLE \n");
      for(int i = 0; i < 255; i++)
      {
        routingEntry* entry = findEntry(i);
        if(entry != NULL)
          printf("MIP: %u -- COST: %u -- NEXT_HOP: %u\n", entry -> mip_address, entry -> cost, entry -> next_hop);
      }
  free(applicationMsg);
}


  free(msg.data);
}

int main(int argc, char* argv[])
{
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
    sendHello();

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
      for(int i = 0; i < amountOfEntries; i++)
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
