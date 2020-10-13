#include "routingTable.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "routing.h"
#include "linkedList.h"
#include "routing_deamon.h"
#include "log.h"
#include <stdbool.h>

extern routingEntry* routingTable[255];
extern bool debug;
extern list* neighbourList;


/*
    This file contains functionality to keep track of the routingtable, and
    the neighbour nodes, making sure that they are both up to date.
*/



/*
    This function sets the cost of all paths with next_hop equal to specified
    mip to infinity (255). This means that these paths does not work anymore.

    @Param  The mip that is to be removed.
*/
void removeFromRouting(u_int8_t mip)
{
  //Look trough entire routing table, find every node with next hop equal to mip.
  for(int i = 0; i < 255; i++)
  {
    if(routingTable[i] != NULL && routingTable[i] -> next_hop == mip)
    {
      if(debug)
      {
        timestamp();
        printf("COST OF %u SET TO INFINITE, NO WAY TO REACH THIS NODE\n", routingTable[i] -> mip_address);
      }
      //Set cost of this node to infinite.
      routingTable[i] -> cost = 255;
    }
  }

  if(debug)
  {
    timestamp();
    printf("ROUTING TABLE AFTER REMOVAL OF ALL PATHS TROUGH %u\n", mip);
    printRoutingTable();
  }
}



/*
    This function just prints out all the current routing entries.
*/
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



/*
    This function removes a node from a list (list of neighbours),
    so that it will no longer be monitored.

    @Param  The mip that is to be removed.
*/
void removeFromList(u_int8_t mip)
{
  if(neighbourList -> head == NULL)
    return;

  node* tempNode = neighbourList -> head;
  node* last;
  last = tempNode;

  //Find the node, remove it.
  while(tempNode != NULL)
  {
     struct neighbourEntry* current = (struct neighbourEntry*) tempNode -> data;
     if(neighbourList -> entries == 1 && current -> mip == mip)
     {
       neighbourList -> head = NULL;
       neighbourList -> entries = 0;
     }

     else if(current -> mip == mip)
     {
       last -> next = tempNode -> next;
       free(tempNode);
       neighbourList -> entries = neighbourList -> entries -1;
       return;
     }

     last = tempNode;
     tempNode = tempNode -> next;
   }

   if(neighbourList -> entries == 0)
   {
     if(debug)
     {
       timestamp();
       printf("THIS NODE IS CURRENTLY ISOLATED -- ENTERING PASSIVE STATE -- WILL NOT SEND KEEP-ALIVE OR UPDATES\n\n");
     }
   }
}



/*
    This function controls the times of last recived keep-alives from neighbours.
    If time is too large, remove them from the neighbour list and from the routingtable.

    If the routing table changes, update all directly connected neighbours.
*/
void controlTime()
{
  bool changed = false;
  if(neighbourList -> head == NULL)
    return;

  node* tempNode = neighbourList -> head;
  while(tempNode != NULL)
  {
     struct neighbourEntry* current = (struct neighbourEntry*) tempNode -> data;
     time_t currentTime;
     time(&currentTime);
     //If a neighbour has not sent keep-alive in 30 sec, consider them dead. 
     if(difftime(currentTime, current -> time) > 30)
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

 if(changed && neighbourList -> entries > 0)
  sendUpdate(0xFF);
}



/*
    This function simply return the value of a certain index in the routingtable.
    The indexes corresponds with mip-addresses.

    @Param  The mip to lookup in the routingtable.
*/
routingEntry* findEntry(u_int8_t mip)
{
  return routingTable[mip];
}



/*
    This function finds the next_hop of path to destination.

    @Param  The destination mip.
    @Return  255 if no route found. next_hop if route found.
*/
int findNextHop(u_int8_t mip)
{
  routingEntry* entry = findEntry(mip);
  if(entry == NULL || entry -> cost == 255)
    return 255;

  return entry -> next_hop;
}



/*
    This function updates the time of last recived keep-alive for a neighbour.

    @Param  The neighbour to reset the time for.
*/
void updateTime(u_int8_t mip)
{
  if(neighbourList -> head == NULL)
    return;

  node* tempNode = neighbourList -> head;
  while(tempNode != NULL)
  {
     struct neighbourEntry* current = (struct neighbourEntry*) tempNode -> data;
     if(current -> mip == mip)
     {
       time(&current -> time);
       return;
     }
     tempNode = tempNode -> next;
   }
}

bool findNeighbour(u_int8_t mip)
{
  node* tempNode = neighbourList -> head;
  while(tempNode != NULL)
  {
     struct neighbourEntry* current = (struct neighbourEntry*) tempNode -> data;
     if(current -> mip == mip)
     {
       return true;
     }
     tempNode = tempNode -> next;
   }

   return false;
}

/*
    This function add a node to the routingtable. It will also add a
    node to the neighbour list if the cost of travel to that node is 1.

    @Param  the mip, the cost of travel and the next_hop.
*/
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
      struct neighbourEntry neighbourEntry;
      neighbourEntry.mip = entry -> mip_address;
      time(&neighbourEntry.time);
      addEntry(neighbourList, &neighbourEntry);
    }
  }
}



/*
    This function updates the estimate of a node if there is already
    a entry for this node in the routingtable. It will also add a
    node to the neighbour list if the cost of travel to that node is 1.

    @Param  the mip, the cost of travel and the next_hop.
*/
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
    struct neighbourEntry neighbourEntry;
    neighbourEntry.mip = entry -> mip_address;
    time(&neighbourEntry.time);
    addEntry(neighbourList, &neighbourEntry);
  }
}
