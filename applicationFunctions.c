#include "applicationFunctions.h"

void sendApplicationMsg(int domainSocket, u_int8_t destination, char* payload)
{
  applicationMsg msg;
  memset(&msg, 0, sizeof(struct applicationMsg));
  msg.address = destination;
  memcpy(&msg.payload, payload, strlen(payload));
//  char* toSend = malloc(sizeof(struct applicationMsg));
//  memcpy(toSend, &msg, sizeof(struct applicationMsg));
  write(domainSocket, &msg, sizeof(msg));
//  free(toSend);
}

applicationMsg readApplicationMsg(int domainSocket)
{
  char* buffer = malloc(sizeof(struct applicationMsg));
  memset(buffer, 0, sizeof(struct applicationMsg));
  applicationMsg msg;
  memset(&msg, 0, sizeof(struct applicationMsg));
  read(domainSocket, buffer, sizeof(struct applicationMsg));
  memcpy(&msg, buffer, strlen(buffer));
  free(buffer);
  return msg;
}
