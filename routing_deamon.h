#ifndef ROUTINGDEAMON
#define ROUTINGDEAMON
#include "routing.h"

routingEntry* findEntry(u_int8_t mip);
int findNextHop(u_int8_t mip);
void sendRoutingMsg(u_int8_t mip_one, char* buffer, int len);
void sendResponse(int destination, u_int8_t next_hop);
void sendHello();
void sendUpdate();
void addToRoutingTable(u_int8_t mip, u_int8_t cost, u_int8_t next);
void updateRoutingEntry(u_int8_t mip, u_int8_t newCost, u_int8_t newNext);
void handleIncomingMsg();
#endif
