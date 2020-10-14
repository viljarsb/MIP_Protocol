#include <string.h> //Memcpy
#include <unistd.h> //Read and write
#include <stdbool.h> //Boolean values
#include <stdio.h> //Printf
#include "log.h" //Timestamp
#include "applicationFunctions.h" //Signatures and defitions of this file.

extern bool debug; //Extern bool value from the main program.
u_int8_t HANDSHAKE[3] = HSK; //Handshake code to identity applicaiton.
u_int8_t HANDSHAKE_REPLY[3] = HSR; //A handshake-reply code, in case need of reply.



/*
    The functions in this file are used to communicate between processes on the
    same node. This file contains functions to send applicaiton data, read
    applicaiton data as well as sending a handshake & handshake-reply to the other process.
*/


/*
    This functions write to a unix domain-socket.

    @Param  a domainsocket-fd, a MIP-address, the application payload as a char*, a time to live and the length of the payload.
    @Return  int, the amount of bytes written to the specified socket-fd
*/
int sendApplicationMsg(int domainSocket, u_int8_t destination, char* payload, u_int8_t ttl, int len)
{
  //Fill in the fields of the struct.
  applicationMsg msg;
  msg.address = destination;

  //If ttl is 0 use the predefined value, if ttl < 0 use ttl = 0, if not use the specified ttl value.
  if(ttl < 0) {msg.ttl = 0;}
  else {msg.ttl = ttl;}


  //Fill in the payload.
  memcpy(msg.payload, payload, len);


  //Write to the socket specified, write 2 bytes for address and ttl + payload length.
  int bytes = write(domainSocket, &msg, 2 + len);

  if(debug)
  {
    timestamp();
    printf("SENT %u BYTES OVER DOMAIN-SOCKET %d\n\n", bytes, domainSocket);
  }

  return bytes;
}



/*
    This functions reads from a unix domain-socket into a specified memory address.

    @Param  a domainsocket-fd and a pointer to a applicationMsg.
    @Return  int, the amount of bytes read from the specified socket-fd.
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



/*
    This function sends a handshake msg over the specified socket.

    @Param  the unix domain-socket to send the handshake over, and a 1-byte value.
*/
void sendHandshake(int domainSocket, u_int8_t value)
{
  handshakeMsg msg;
  memcpy(msg.type, HANDSHAKE, sizeof(HANDSHAKE));
  msg.value = value;
  char* buffer = calloc(1, sizeof(handshakeMsg));
  memcpy(buffer, &msg, sizeof(handshakeMsg));
  int bytes = sendApplicationMsg(domainSocket, 0, buffer, 0, sizeof(handshakeMsg));
  free(buffer);

  if(debug)
  {
    timestamp();
    printf("SENT HANDSHAKE OVER DOMAIN-SOCKET %d\n\n", domainSocket);
  }
}



/*
    This function sends a handshake-reply msg over the specified socket.

    @Param  the unix domain-socket to send the handshake over, and a 1-byte value.
*/
void sendHandshakeReply(int domainSocket, u_int8_t value)
{
  handshakeMsg msg;
  memcpy(msg.type, HANDSHAKE_REPLY, sizeof(HANDSHAKE_REPLY));
  msg.value = value;
  char* buffer = calloc(1, sizeof(handshakeMsg));
  memcpy(buffer, &msg, sizeof(handshakeMsg));
  int bytes = sendApplicationMsg(domainSocket, 0, buffer, 0, sizeof(handshakeMsg));
  free(buffer);
  if(debug)
  {
    timestamp();
    printf("SENT HANDSHAKE-REPLY DOMAIN-SOCKET %d\n\n", domainSocket);
  }
}
