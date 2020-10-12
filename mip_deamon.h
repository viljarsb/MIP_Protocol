#ifndef MIPDEAMON
#define MIPDEAMON

void handleRawPacket(int socket_fd, int pingApplication, int routingApplication);
void handleApplicationPacket(int activeApplication, int routingApplication, int socket_fd);
void sendForwardingReuest(int routingSocket, u_int8_t mip);
void handleRoutingPacket(int socket_fd, int routingApplication);

#endif
