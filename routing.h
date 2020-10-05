#ifndef ROUTING_
#define ROUTING_
#include <sys/socket.h>

//Routing related codes
#define REQ {0x52, 0x45, 0x51}
#define RSP {0x52, 0x53, 0x50}
#define UPD {0x55, 0x50, 0x44}
#define HEL {0x48, 0x45, 0x4c}

typedef struct helloMsg helloMsg;
typedef struct routingEntry routingEntry;
typedef struct updateStructure updateStructure;
typedef struct routingQuery routingQuery;

struct routingQuery
{
  u_int8_t type[3];
  u_int8_t mip;
};

struct helloMsg
{
  u_int8_t type[3];
};

struct updateStructure
{
  u_int8_t type[3];
  u_int8_t amount;
  void* data;
};

struct routingEntry
{
  u_int8_t mip_address;
  u_int8_t cost;
  u_int8_t next_hop;
};
void SendForwardingRequest(int routingSocket, u_int8_t mip);
void sendRoutingBroadcast(int socket_fd, char* payload, int len);

#endif
