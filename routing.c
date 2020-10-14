#include <stdbool.h> //boolean values.
#include <stdio.h> //printf.
#include <string.h> //memcpy.
#include "routing.h" //Signatures of this file.
#include "applicationFunctions.h" //Send applicationMsgs between processes.
#include "rawFunctions.h" //Send data over raw-socket.
#include "log.h" //timestamp
#include "protocol.h" //mip-header..

extern bool debug; //Extern debug flag.
extern u_int8_t MY_MIP_ADDRESS; //Extern MY_MIP_ADDRESS.

//Routing codes of requests and responses.
const u_int8_t REQUEST[3] = REQ;
const u_int8_t RESPONSE[3] = RSP;


/*
    This file contains functionality of the forwarding engine of the mip-deamon.
*/


/*
    This functions send a request to the routing-deamon to ask for the next_hop.

    @Param  The socket used to talk to the routingDeamon, the mip to look for the next path of.
*/
void sendForwardingReuest(int routingSocket, u_int8_t mip)
{
  if(debug)
  {
    timestamp();
    printf("SENDING REQUEST TO ROUTING-DEAMON FOR NEXT HOP ON PATH TO %u\n", mip);
  }

  //Create a query and fill in data (the mip we want the next jump of).
  struct routingQuery query;
  memcpy(query.type, REQUEST, sizeof(REQUEST));
  query.mip = mip;

  //Create a buffer and fill it with the query data.
  char* buffer = calloc(1, sizeof(routingQuery));
  memcpy(buffer, &query, sizeof(routingQuery));

  //send the query data.
  sendApplicationMsg(routingSocket, MY_MIP_ADDRESS, buffer, -1, sizeof(routingQuery));
  free(buffer);
}



/*
    This functions sends ROUTING-SDUs.

    @Param  The raw-socket, the payload, the length of the payload and the destination mip-address.
*/
void sendRoutingSdu(int socket_fd, char* payload, int len, u_int8_t dst_mip)
{
  if(debug)
  {
    timestamp();
    printf("SENDING ROUTING-SDU -- MIP SRC: %u -- MIP DEST: %u\n", MY_MIP_ADDRESS, dst_mip);
  }

  //Create and fill in buffer.
  char* buffer = calloc(1, len);
  memcpy(buffer, payload, len);
  free(payload);

  //Create and fill in mip-header.
  mip_header* mip_header = calloc(1, sizeof(struct mip_header));
  mip_header -> src_addr = MY_MIP_ADDRESS;
  mip_header -> dst_addr = dst_mip;
  mip_header -> sdu_type = ROUTING;
  mip_header -> ttl = BROADCAST_TTL_MIP;
  mip_header -> sdu_length = len;

  sendData(socket_fd, mip_header, buffer, dst_mip);
}
