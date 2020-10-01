#include "applicationFunctions.h"
#include <string.h>
#include <unistd.h>
extern int debug;

/*
    This functions write to a unix domain socket.
    @Param  a domainsocket fd, a destination (MIP) and the application payload.
*/
int sendApplicationMsg(int domainSocket, u_int8_t destination, char* payload, int len)
{
  applicationMsg msg;
  msg.address = destination;
  msg.TTL = 10;
  memcpy(msg.payload, payload, len);
  int bytes = write(domainSocket, &msg, 2 + len);
  return bytes;
}

/*
    This functions reads from a unix domain socket.
    @Param  a domainsocket fd
    @Return   a applicationMsg struct that contains the dst (MIP) and the payload.
*/
int readApplicationMsg(int domainSocket, applicationMsg* appMsg)
{
  int rc = read(domainSocket, appMsg, sizeof(struct applicationMsg));
  return rc;
}
