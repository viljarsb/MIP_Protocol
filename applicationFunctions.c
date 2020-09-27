#include "applicationFunctions.h"
#include <string.h>
#include <unistd.h>
extern int debug;

/*
    This functions write to a unix domain socket.
    @Param  a domainsocket fd, a destination (MIP) and the application payload.
*/
void sendApplicationMsg(int domainSocket, u_int8_t destination, char* payload, int len)
{
  applicationMsg msg;
  memset(&msg, 0, sizeof(struct applicationMsg));
  msg.address = destination;
  msg.TTL = 10;
  memcpy(msg.payload, payload, len);
  write(domainSocket, &msg, sizeof(struct applicationMsg));
}

/*
    This functions reads from a unix domain socket.
    @Param  a domainsocket fd
    @Return   a applicationMsg struct that contains the dst (MIP) and the payload.
*/
applicationMsg readApplicationMsg(int domainSocket)
{
  char* buffer = calloc(1, sizeof(struct applicationMsg));
  applicationMsg msg;
  memset(&msg, 0, sizeof(struct applicationMsg));
  int rc = read(domainSocket, buffer, sizeof(struct applicationMsg));
  memcpy(&msg, buffer, rc);
  free(buffer);
  return msg;
}
