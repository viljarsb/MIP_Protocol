#ifndef ROUTINGDEAMON
#define ROUTINGDEAMON

void sendResponse(int destination, u_int8_t next_hop);
void sendHello();
void sendUpdate();
void handleIncomingMsg();
#endif
