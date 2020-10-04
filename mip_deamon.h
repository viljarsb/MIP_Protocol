#ifndef MIPDEAMON
#define MIPDEAMON
#include "linkedList.h"
#include "applicationFunctions.h"
#include "protocol.h"

extern u_int8_t MY_MIP_ADDRESS;
extern bool debug;
extern list* interfaces;
extern list* arpCache;

void handleRawPacket(int socket_fd, int pingApplication, int routingApplication);
void handleApplicationPacket(int activeApplication, int routingApplication, int socket_fd);
void SendForwardingRequest(int routingSocket, u_int8_t mip);

#endif
