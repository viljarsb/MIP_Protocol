#include "routing.h"
#include "applicationFunctions.h"
#include "rawFunctions.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "protocol.h"

extern bool debug;
extern u_int8_t MY_MIP_ADDRESS;
const u_int8_t REQUEST[3] = REQ;
const u_int8_t RESPONSE[3] = RSP;
const u_int8_t UPDATE[3] = UPD;
const u_int8_t HELLO[3] = HEL;

/*
    This file contains functionality of the forwarding engine of the mip-deamon.
*/



/*
    This functions send a request to the routing-deamon to ask for the next_hop.

    @Param  The socket used to talk to the routingDeamon, the mip to look for.
*/
void sendForwardingReuest(int routingSocket, u_int8_t mip)
{
  if(debug)
  {
    timestamp();
    printf("SENDING REQUEST TO ROUTING-DEAMON FOR NEXT HOP ON PATH TO %u\n", mip);
  }

  struct routingQuery query;
  char* buffer = calloc(1, sizeof(routingQuery));
  memcpy(query.type, REQUEST, sizeof(REQUEST));
  query.mip = mip;
  memcpy(buffer, &query, sizeof(routingQuery));
  sendApplicationMsg(routingSocket, MY_MIP_ADDRESS, buffer, -1, sizeof(routingQuery));
  free(buffer);
}



/*
    This functions broadcasts ROUTING-SDUs over the broadcast channel.

    @Param  The raw-socket, the payload and the length of the payload.
*/
void sendRoutingSdu(int socket_fd, char* payload, int len, u_int8_t dst_mip)
{
  if(debug)
  {
    timestamp();
    printf("SENDING ROUTING-SDU -- MIP SRC: %u -- MIP DEST: %u\n", MY_MIP_ADDRESS, dst_mip);
  }

  mip_header* mip_header = calloc(1, sizeof(struct mip_header));
  char* buffer = calloc(1, len);
  memcpy(buffer, payload, len);
  free(payload);

  mip_header -> src_addr = MY_MIP_ADDRESS;
  mip_header -> dst_addr = dst_mip;
  mip_header -> sdu_type = ROUTING;
  mip_header -> ttl = BROADCAST_ttl_MIP;
  mip_header -> sdu_length = len;

  sendData(socket_fd, mip_header, buffer, dst_mip);
}
