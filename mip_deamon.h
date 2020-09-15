#ifndef MIPDEAMON
#define MIPDEAMON
#include "linkedList.h"
#include "applicationFunctions.h"

extern u_int8_t MY_MIP_ADDRESS;
extern int debug;
extern list* interfaces;
extern list* arpCache;
extern applicationMsg* unsent;

void sendArpResponse(int socket_fd, u_int8_t dst_mip);
void sendArpBroadcast(int socket_fd, list* interfaces, u_int8_t lookup);
void sendApplicationData(int socket_fd, applicationMsg* msg, u_int8_t mipDst);
void handleRawPacket(int socket_fd, int activeApplication);
void handleApplicationPacket(int activeApplication, int socket_fd);

#endif
