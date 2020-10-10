#ifndef APPLICATIONFUNCTIONS
#define APPLICATIONFUNCTIONS
#define TTL_UNDEFINED 15;
#include <stdlib.h>

typedef struct applicationMsg applicationMsg;

struct applicationMsg
{
  u_int8_t address;
  u_int8_t TTL;
  char payload[1024];
}__attribute__ ((packed));

int sendApplicationMsg(int domainSocket, u_int8_t destination, char* payload, u_int8_t ttl, int len);
int readApplicationMsg(int domainSocket, applicationMsg* appMsg);

#endif
