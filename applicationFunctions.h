#ifndef APPLICATIONFUNCTIONS
#define APPLICATIONFUNCTIONS
#define TTL_UNDEFINED 15;
#define HSK {0x48, 0x53, 0x4b} //HANDSHAKE
#include <stdlib.h>

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
};

int sendApplicationMsg(int domainSocket, u_int8_t destination, char* payload, u_int8_t ttl, int len);
int readApplicationMsg(int domainSocket, applicationMsg* appMsg);
void sendHandshake(int domainSocket, u_int8_t value);
#endif
