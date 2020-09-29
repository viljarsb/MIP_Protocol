#ifndef APPLICATIONFUNCTIONS
#define APPLICATIONFUNCTIONS
#include <stdlib.h>

typedef struct applicationMsg applicationMsg;

struct applicationMsg
{
  u_int8_t address;
  u_int8_t TTL;
  void* payload;
};


void sendApplicationMsg(int domainSocket, u_int8_t destination, char* payload, int len);
int readApplicationMsg(int domainSocket, applicationMsg* appMsg);

#endif
