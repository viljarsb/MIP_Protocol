#ifndef APPLICATIONFUNCTIONS
#define APPLICATIONFUNCTIONS
#include <stdlib.h> //u_int8_t

#define ttl_UNDEFINED 15; //Use if no ttl is defined, no need for larger value. MIP-Headers can only hold 4-bit ttl.
#define HSK {0x48, 0x53, 0x4b} //HANDSHAKE
#define HSR {0x48, 0x53, 0x52} //HANDSHAKE-REPLY

typedef struct applicationMsg applicationMsg;
typedef struct handshakeMsg handshakeMsg;

//Struct to hold data sent between applications running on the same host.
struct applicationMsg
{
  u_int8_t address;
  u_int8_t ttl;
  char payload[1024];
}__attribute__ ((packed));

//A struct to hold data of a handshake-process.
struct handshakeMsg
{
  u_int8_t type[3];
  u_int8_t value;
}__attribute__ ((packed));

int sendApplicationMsg(int domainSocket, u_int8_t destination, char* payload, u_int8_t ttl, int len);
int readApplicationMsg(int domainSocket, applicationMsg* appMsg);
void sendHandshake(int domainSocket, u_int8_t value);
void sendHandshakeReply(int domainSocket, u_int8_t value);
#endif
