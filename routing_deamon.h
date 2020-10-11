#ifndef ROUTINGDEAMON
#define ROUTINGDEAMON

void sendResponse(int destination, u_int8_t next_hop);
void sendHello(u_int8_t dst_mip);
void sendUpdate(u_int8_t dst_mip);
void handleIncomingMsg();
void sendKeepAlive(u_int8_t dst_mip);
#endif
