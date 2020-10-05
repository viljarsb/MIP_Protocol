#include "routing.h"
#include "applicationFunctions.h"
#include "rawFunctions.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


extern bool debug;
extern u_int8_t MY_MIP_ADDRESS;
const u_int8_t REQUEST[3];
const u_int8_t RESPONSE[3];
const u_int8_t UPDATE[3];
const u_int8_t HELLO[3];


void SendForwardingRequest(int routingSocket, u_int8_t mip)
{
  struct routingQuery query;
  char* buffer = calloc(1, sizeof(routingQuery));
  memcpy(query.type, REQUEST, sizeof(REQUEST));
  query.mip = mip;
  memcpy(buffer, &query, sizeof(routingQuery));
  sendApplicationMsg(routingSocket, MY_MIP_ADDRESS, buffer, sizeof(routingQuery));
  free(buffer);
}

void sendRoutingBroadcast(int socket_fd, char* payload, int len)
{
  if(debug)
    printf("SENDING ROUTING-SDU -- MIP SRC: %u -- MIP DEST: %u\n", MY_MIP_ADDRESS, 0xFF);
  mip_header* mip_header = calloc(1, sizeof(struct mip_header));
  char* buffer = calloc(1, len);
  memcpy(buffer, payload, len);
  free(payload);

  mip_header -> src_addr = MY_MIP_ADDRESS;
  mip_header -> dst_addr = 0xFF;
  mip_header -> sdu_type = ROUTING;
  mip_header -> ttl = 1;
  mip_header -> sdu_length = len;

  sendApplicationData(socket_fd, mip_header, buffer, 0xFF);
}
