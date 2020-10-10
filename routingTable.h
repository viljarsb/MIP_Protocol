#ifndef ROUTING_TABLE
#define ROUTING_TABLE
#include "routing.h"

struct timerEntry
{
  u_int8_t mip;
  time_t time;
};

void removeFromRouting(u_int8_t mip);
void removeFromList(u_int8_t mip);
void controlTime();
routingEntry* findEntry(u_int8_t mip);
int findNextHop(u_int8_t mip);
void updateTime(u_int8_t mip);
void addToRoutingTable(u_int8_t mip, u_int8_t cost, u_int8_t next);
void updateRoutingEntry(u_int8_t mip, u_int8_t newCost, u_int8_t newNext);
void printRoutingTable();
#endif
