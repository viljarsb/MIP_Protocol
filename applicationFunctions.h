#ifndef APPLICATIONFUNCTIONS
#define APPLICATIONFUNCTIONS

#include <stdlib.h>

#define TTL_UNDEFINED 15; //Use if no ttl is defined.
#define HSK {0x48, 0x53, 0x4b} //HANDSHAKE
#define HSR {0x41, 0x43, 0x4b} //HANDSHAKE REPLY

typedef struct applicationMsg applicationMsg;
typedef struct handshakeMsg handshakeMsg;

struct applicationMsg
{
  u_int8_t address;
  u_int8_t TTL;
  char payload[1024];
}__attribute__ ((packed));

struct handshakeMsg
{
  u_int8_t type[3];
  u_int8_t value;
}__attribute__ ((packed));

int sendApplicationMsg(int domainSocket, u_int8_t destination, char* payload, u_int8_t ttl, int len);
int readApplicationMsg(int domainSocket, applicationMsg* appMsg);
void sendHandshake(int domainSocket, u_int8_t value);
#endif
