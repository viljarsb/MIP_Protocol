#ifndef APPLICATIONFUNCTIONS
#define APPLICATIONFUNCTIONS
#include "protocol.h"
typedef struct applicationMsg applicationMsg;


struct applicationMsg
{
  u_int8_t address;
  char payload[1024];
};

void sendApplicationMsg(int domainSocket, u_int8_t destination, char* payload);
applicationMsg readApplicationMsg(int domainSocket);

#endif
