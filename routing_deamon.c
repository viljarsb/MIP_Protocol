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
bool debug = true;

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
  int bytes = sendApplicationMsg(routingSocket, mip_one, buffer, len);
  if(debug)
    printf("SENDT %d BYTES TO MIP_DEAMON\n\n", bytes);
}

void sendResponse(int destination, u_int8_t next_hop)
{
  if(debug)
    printf("SENDING ROUTING-RESPONSE FOR MIP: %u\n", destination);

  routingQuery query;

  memcpy(query.type, RESPONSE, sizeof(RESPONSE));
  query.mip = next_hop;

  char* buffer = calloc(1, sizeof(routingQuery));
  memcpy(buffer, &query, sizeof(routingQuery));
  sendRoutingMsg(destination, buffer, sizeof(routingQuery));
  free(buffer);
}

void sendHello()
{
  printf("SENDING HELLO-MSG TO NEIGHBOURS\n");
  char* buffer = calloc(1, 3);
  helloMsg msg;
  memcpy(&msg.type, HELLO, sizeof(HELLO));
  memcpy(buffer, &msg, sizeof(HELLO));
  sendRoutingMsg(MY_MIP_ADDRESS, buffer, sizeof(HELLO));
  free(buffer);
}

void sendUpdate()
{
  if(debug)
    printf("SENDING UPDATE-MSG TO NEIGHBOURS\n\n");

  char* buffer = calloc(1, 3);
  updateStructure updateStructure;
  memcpy(updateStructure.type, UPDATE, sizeof(UPDATE));
  memcpy(buffer, updateStructure.type, sizeof(UPDATE));

  updateStructure.amount = 0;
  updateStructure.data = malloc(0);

  for(int i = 0; i < 255; i++)
  {
    if(findEntry(i) != NULL)
    {
      updateStructure.amount = updateStructure.amount + 1;
      updateStructure.data = realloc(updateStructure.data, sizeof(routingEntry) * updateStructure.amount);
      memcpy(updateStructure.data + (updateStructure.amount -1) * sizeof(routingEntry), &*routingTable[i], sizeof( routingEntry));
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
    if(debug)
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
  if(debug)
    printf("UPDATING COST OF MIP: %u TO %u WITH NEXT_HOP: %u\n\n", mip, newCost, newNext);
  routingEntry* entry = findEntry(mip);
  entry -> cost = newCost;
  entry -> next_hop = newNext;
}

void handleIncomingMsg()
{
  applicationMsg* applicationMsg = calloc(1, sizeof(struct applicationMsg));
  int rc = readApplicationMsg(routingSocket, applicationMsg);

  if(debug)
    printf("RECIEVIED %d BYTES FROM MIP-DEAMON\n", rc);

  if(memcmp(REQUEST, applicationMsg -> payload, sizeof(REQUEST)) == 0)
  {
    routingQuery query;
    memcpy(&query, applicationMsg -> payload, sizeof(routingQuery));
    if(debug)
      printf("RECIEVIED ROUTING-REQUEST FOR DESTINATION %u\n\n", query.mip);
    u_int8_t next_hop = findNextHop(query.mip);
    sendResponse(query.mip, next_hop);
  }

  else if(memcmp(HELLO, applicationMsg -> payload, sizeof(HELLO)) == 0)
  {
    if(debug)
      printf("RECIEVIED HELLO-BROADCAST FROM MIP: %u -- ADDING TO ROUTINGTABLE\n", applicationMsg -> address);
    addToRoutingTable(applicationMsg -> address, 1, applicationMsg -> address);
    sendUpdate();
  }

  else if(memcmp(UPDATE, applicationMsg -> payload, sizeof(UPDATE)) == 0)
  {

    printf("RECIEVIED ROUTING-UPDATE -- UPDATING ROUTINGTABLE\n");
    bool changed = false;

    updateStructure updateStructure;
    memcpy(&updateStructure, applicationMsg -> payload, sizeof(u_int8_t) * 4);
    updateStructure.data = malloc(updateStructure.amount * sizeof(routingEntry));
    memcpy(updateStructure.data, applicationMsg -> payload + 4, updateStructure.amount * sizeof(routingEntry));

    int currentPos = 0;
    int counter = 0;
    routingEntry entry;
    while(counter < updateStructure.amount)
    {
      memcpy(&entry, updateStructure.data + currentPos, sizeof(routingEntry));
      currentPos = currentPos + sizeof(routingEntry);

      if(findEntry(entry.mip_address) == NULL)
      {
        addToRoutingTable(entry.mip_address, entry.cost + 1, applicationMsg -> address);
        changed = true;
      }

      else if(findEntry(entry.mip_address) != NULL)
      {
        if(entry.cost + 1 < findEntry(entry.mip_address) -> cost)
        {
          updateRoutingEntry(entry.mip_address, entry.cost + 1, applicationMsg -> address);
          changed = true;
          if(debug)
            printf("UPDATED NEXT-HOP OF %u\n\n", entry.mip_address);
        }
      }

      counter = counter +1;
    }

    if(changed)
    {
      if(debug)
        printf("UPDATED COMPLETE -- TABLE WAS ALTERED -- SENDING UPDATE TO NEIGHBOURS\n\n");
      sendUpdate();
    }

    else
    {
      if(debug)
      {
        printf("UPDATED COMPLETE -- TABLE WAS NOT ALTERED\n\n");
        printf("CURRENT ROUTINGTABLE \n");
        for(int i = 0; i < 255; i++)
        {
          routingEntry* entry = findEntry(i);
          if(entry != NULL)
            printf("MIP: %u -- COST: %u -- NEXT_HOP: %u\n", entry -> mip_address, entry -> cost, entry -> next_hop);
        }
      }
    }
  }

  free(applicationMsg);
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

  if(debug)
    printf("ROUTING_DEAMON STARTED ON HOST %u\n\n", MY_MIP_ADDRESS);

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
         if(events[i].events & EPOLLIN)
         {
            if(events[i].data.fd == routingSocket)
           {
             handleIncomingMsg();
           }
         }

       }
     }
}
