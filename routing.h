#ifndef ROUTING_
#define ROUTING_
#include <stdlib.h> //u_int8_t

//Routing related codes
#define REQ {0x52, 0x45, 0x51} //REQUEST
#define RSP {0x52, 0x53, 0x50} //RESPONSE
#define UPD {0x55, 0x50, 0x44} //UPDATE
#define HEL {0x48, 0x45, 0x4c} //HELLO
#define ALV {0x41, 0x4c, 0x56} //KEEP-ALIVE

typedef struct helloMsg helloMsg;
typedef struct keepAlive keepAlive;
typedef struct routingEntry routingEntry;
typedef struct updateStructure updateStructure;
typedef struct routingQuery routingQuery;

//Struct to store routing queries, for requests and responses.
//Contains the type of the msg and the mip (either the one to lookup or the next hop.)
struct routingQuery
{
  u_int8_t type[3];
  u_int8_t mip;
};

//A hello msg, only contains type. No more info needed.
struct helloMsg
{
  u_int8_t type[3];
};

//Same as hello, just for a cleaner implementation.
struct keepAlive
{
  u_int8_t type[3];
};

//Update struct, meant to store update data from a routing deamon.
//Contains the type, the amount of nodes in the update msg and the pointer to the data.
struct updateStructure
{
  u_int8_t type[3];
  u_int8_t amount;
  void* data;
};

//A entry in the routing table.
struct routingEntry
{
  u_int8_t mip_address;
  u_int8_t cost;
  u_int8_t next_hop;
};


void sendForwardingReuest(int routingSocket, u_int8_t mip);
void sendRoutingSdu(int socket_fd, char* payload, int len, u_int8_t dst_mip);
#endif
