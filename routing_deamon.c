#include <stdio.h>
#include "unistd.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdbool.h>
#include "epollFunctions.h"
#include "socketFunctions.h"
#include "linkedList.h"
#include "protocol.h"
#include <time.h>
#include <sys/timerfd.h>
#include "applicationFunctions.h"
#include "routingTable.h"
#include "routing_deamon.h"

routingEntry* routingTable[255];
u_int8_t REQUEST[3] = REQ;
u_int8_t RESPONSE[3] = RSP;
u_int8_t UPDATE[3] = UPD;
u_int8_t HELLO[3] = HEL;
u_int8_t MY_MIP_ADDRESS;
int routingSocket;
bool debug = true;
list* timeList;

void sendHello()
{
  printf("SENDING HELLO-MSG TO NEIGHBOURS OVER 255\n");
  char* buffer = calloc(1, 3);
  helloMsg msg;
  memcpy(&msg.type, HELLO, sizeof(HELLO));
  memcpy(buffer, &msg, sizeof(HELLO));
  sendApplicationMsg(routingSocket, MY_MIP_ADDRESS, buffer, 1, sizeof(HELLO));
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
  sendApplicationMsg(routingSocket, MY_MIP_ADDRESS, buffer, 1, 4 + (updateStructure.amount * sizeof(routingEntry)));
}

void sendResponse(int destination, u_int8_t next_hop)
{
  if(debug)
    printf("SENDING ROUTING-RESPONSE FOR MIP: %u -- NEXT HOP: %u\n", destination, next_hop);

  routingQuery query;

  memcpy(query.type, RESPONSE, sizeof(RESPONSE));
  query.mip = next_hop;

  char* buffer = calloc(1, sizeof(routingQuery));
  memcpy(buffer, &query, sizeof(routingQuery));
  sendApplicationMsg(routingSocket, destination, buffer, -1, sizeof(routingQuery));
}

void alarm_()
{
  if(debug)
    printf("SENDING KEEP-ALIVE -- INFORMING NEIGHBOURS OF MY PRESENCE\n\n");
  sendHello();
  controlTime();
}

void handleIncomingMsg()
{
  applicationMsg* applicationMsg = calloc(1, sizeof(struct applicationMsg));
  int rc = readApplicationMsg(routingSocket, applicationMsg);

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
    if(findEntry(applicationMsg -> address) == NULL)
    {
      if(debug)
        printf("RECIEVIED HELLO-BROADCAST FROM MIP: %u -- ADDING TO ROUTINGTABLE\n", applicationMsg -> address);
      addToRoutingTable(applicationMsg -> address, 1, applicationMsg -> address);
      sendUpdate();
    }

    else
    {
      if(debug)
        printf("RECIVED KEEP-ALIVE BROADCAST FROM: %u\n\n", applicationMsg -> address);
      updateTime(applicationMsg -> address);
    }
  }

  else if(memcmp(UPDATE, applicationMsg -> payload, sizeof(UPDATE)) == 0)
  {

    if(debug)
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
  timeList = createLinkedList(sizeof(struct timerEntry));
  if(argc == 2)
  {
    domainPath = argv[1];
  }

  if(debug)
    printf(" ** ROUTING_DEAMON STARTED ** \n\n");

  routingSocket = createDomainClientSocket(domainPath);
  u_int8_t temp = ROUTING;
  write(routingSocket, &temp, sizeof(u_int8_t));
  read(routingSocket, &MY_MIP_ADDRESS, sizeof(u_int8_t));

  addToRoutingTable(MY_MIP_ADDRESS, 0, MY_MIP_ADDRESS);

  if(debug)
   printf("SENDING INITIAL HELLO-BROADCAST\n\n");
  sendHello();

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
       perror("epoll_create");
       return -1;
     }

     int timer_fd;
     struct itimerspec ts;
     memset(&ts, 0, sizeof(struct itimerspec));
     ts.it_value.tv_sec = 5;
     ts.it_value.tv_nsec = 0;
     timer_fd = timerfd_create(CLOCK_REALTIME, 0);

     if(timer_fd == -1)
     {
       printf("timerfd_create");
       exit(EXIT_FAILURE);
     }

     if(timerfd_settime(timer_fd, 0, &ts, NULL) < 0)
     {
       printf("timerfd_settime");
       exit(EXIT_FAILURE);
     }


    struct epoll_event events[1];
    int amountOfEntries;
    addEpollEntry(routingSocket, epoll_fd);
    addEpollEntry(timer_fd, epoll_fd);

    //Loop forever.
    while(true)
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

           else if(events[i].data.fd == timer_fd)
           {
             alarm_();
             timerfd_settime(timer_fd, 0, &ts, NULL);
           }
         }
      }
    }
}
