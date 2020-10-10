#include "routingTable.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "routing.h"
#include "linkedList.h"
#include "routing_deamon.h"
#include "log.h"

extern routingEntry* routingTable[255];
extern bool debug;
extern list* timeList;

void removeFromRouting(u_int8_t mip)
{
  for(int i = 0; i < 255; i++)
  {
    if(routingTable[i] != NULL && routingTable[i] -> next_hop == mip)
      updateRoutingEntry(routingTable[i] -> mip_address, 255, routingTable[i] -> next_hop);
  }
}

void printRoutingTable()
{
  timestamp();
  printf("CURRENT ROUTINGTABLE\n");
  for(int i = 0; i < 255; i++)
  {
    routingEntry* entry = findEntry(i);
    if(entry != NULL)
      printf("             MIP: %u -- COST: %u -- NEXT_HOP: %u\n", entry -> mip_address, entry -> cost, entry -> next_hop);
  }
}

void removeFromList(u_int8_t mip)
{
  if(timeList -> head == NULL)
    return;

  node* tempNode = timeList -> head;
  node* last = malloc(sizeof(struct timerEntry));
  while(tempNode != NULL)
  {
     struct timerEntry* current = (struct timerEntry*) tempNode -> data;
     if(tempNode == timeList -> head)
     {
       timeList -> head = NULL;
       return;
     }

     if(current -> mip == mip)
     {
       last -> next = tempNode -> next;
     }
     last = tempNode;
     tempNode = tempNode -> next;
 }
}

void controlTime()
{
  bool changed = false;
  if(timeList -> head == NULL)
    return;

  node* tempNode = timeList -> head;
  while(tempNode != NULL)
  {
     struct timerEntry* current = (struct timerEntry*) tempNode -> data;
     time_t currentTime;
     time(&currentTime);
     if(difftime(currentTime, current -> time) > 10)
     {
       if(debug)
       {
         timestamp();
         printf("NO KEEP-ALIVE FROM %d RECIVED WITHIN TIMEFRAME\n", current -> mip);
         timestamp();
         printf("REMOVED ALL PATHS TROUGH %u FROM ROUTING TABLE\n\n", current -> mip);
       }

       removeFromRouting(current -> mip);
       removeFromList(current -> mip);
       changed = true;
     }
     tempNode = tempNode -> next;
 }

 if(changed)
  sendUpdate();

}

routingEntry* findEntry(u_int8_t mip)
{
  return routingTable[mip];
}

int findNextHop(u_int8_t mip)
{
  routingEntry* entry = findEntry(mip);
  if(entry == NULL || entry -> cost == 255)
    return 255;

  return entry -> next_hop;
}

void updateTime(u_int8_t mip)
{
  if(timeList -> head == NULL)
    return;

  node* tempNode = timeList -> head;
  while(tempNode != NULL)
  {
     struct timerEntry* current = (struct timerEntry*) tempNode -> data;
     if(current -> mip == mip)
     {
       time(&current -> time);
       return;
     }
     tempNode = tempNode -> next;
 }
}

void addToRoutingTable(u_int8_t mip, u_int8_t cost, u_int8_t next)
{
  if(findEntry(mip) == NULL)
  {
    if(debug)
    {
      timestamp();
      printf("ADDED %u TO ROUTING-TABLE -- COST: %u -- NEXT HOP: %u\n\n", mip, cost, next);
    }

    routingEntry* entry = malloc(sizeof(routingEntry));
    entry -> mip_address = mip;
    entry -> cost = cost;
    entry -> next_hop = next;
    routingTable[mip] = entry;

    //This is a neighbour over a direct link
    if(entry -> cost == 1)
    {
      struct timerEntry* timerEntry = malloc(sizeof(struct timerEntry));
      timerEntry -> mip = entry -> mip_address;
      time(&timerEntry -> time);
      addEntry(timeList, timerEntry);
    }
  }

  else
  {
    updateRoutingEntry(mip, cost, next);
  }
}

void updateRoutingEntry(u_int8_t mip, u_int8_t newCost, u_int8_t newNext)
{
  if(debug)
  {
    timestamp();
    printf("UPDATING COST OF MIP: %u TO %u WITH NEXT_HOP: %u\n\n", mip, newCost, newNext);
  }
  routingEntry* entry = findEntry(mip);
  entry -> cost = newCost;
  entry -> next_hop = newNext;

  if(entry -> cost == 1)
  {
    struct timerEntry* timerEntry = malloc(sizeof(struct timerEntry));
    timerEntry -> mip = entry -> mip_address;
    time(&timerEntry -> time);
    addEntry(timeList, timerEntry);
  }
}
