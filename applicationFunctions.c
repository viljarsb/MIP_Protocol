#include "applicationFunctions.h"
#include <string.h> //Memcpy
#include <unistd.h> //Read and write
#include <stdbool.h> //Boolean values
#include <stdio.h> //Printf
#include "log.h" //Timestamp
extern bool debug; //Extern bool value from the main program. 

/*
    This functions write to a unix domain-socket.
    @Param  a domainsocket-fd, a MIP-address, the application payload as a char*, a time to live and the length of the payload.
    @Retrun  int, the amount of bytes written to the specified socket-fd
*/
int sendApplicationMsg(int domainSocket, u_int8_t destination, char* payload, u_int8_t ttl, int len)
{
  //Fill in the fields of the struct.
  applicationMsg msg;
  msg.address = destination;

  //If ttl is 0 use the redefined value, if ttl = -1 use ttl = 0, if not use the specified TTL.
  if(ttl == 0) {msg.TTL = TTL_UNDEFINED;}
  else if(ttl == -1) {msg.TTL = 0;}
  else {msg.TTL = ttl;}
  memcpy(msg.payload, payload, len);
  int bytes = write(domainSocket, &msg, 2 + len);

  if(debug)
  {
    timestamp();
    printf("SENDT %u BYTES OVER DOMAIN-SOCKET %d\n\n", bytes, domainSocket);
  }

  return bytes;
}

/*
    This functions reads from a unix domain socket into a specified memory address.
    @Param  a domainsocket-fd and a pointer to a applicationMsg.
    @Retrun  int, the amount of bytes written to the specified socket-fd.
*/
int readApplicationMsg(int domainSocket, applicationMsg* appMsg)
{
  //Read into the applicationMsg struct memory location.
  int bytes = read(domainSocket, appMsg, sizeof(struct applicationMsg));

  if(debug)
  {
    timestamp();
    printf("RECIVED %u BYTES OVER DOMAIN-SOCKET %d\n", bytes, domainSocket);
  }

  return bytes;
}
