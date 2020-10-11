#ifndef ROUTING_
#define ROUTING_
#include <stdlib.h> //

//Routing related codes
#define REQ {0x52, 0x45, 0x51} //REQUEST
#define RSP {0x52, 0x53, 0x50} //RESPONSE
#define UPD {0x55, 0x50, 0x44} //UPDATE
#define HEL {0x48, 0x45, 0x4c} //HELLO
#define ALV {0x41, 0x4c, 0x56} //KEEP-ALIVE
#define HSK {0x48, 0x53, 0x4b} //HANDSHAKE


typedef struct helloMsg helloMsg;
typedef struct keepAlive keepAlive;
typedef struct routingEntry routingEntry;
typedef struct updateStructure updateStructure;
typedef struct routingQuery routingQuery;
typedef handshakeMsg handshakeMsg;

struct routingQuery
{
  u_int8_t type[3];
  u_int8_t mip;
};

struct helloMsg
{
  u_int8_t type[3];
  u_int8_t adr;
};

struct keepAlive
{
  u_int8_t type[3];
  u_int8_t adr;
};

struct updateStructure
{
  u_int8_t type[3];
  u_int8_t adr;
  u_int8_t amount;
  void* data;
};

struct routingEntry
{
  u_int8_t mip_address;
  u_int8_t cost;
  u_int8_t next_hop;
};

struct handshakeMsg
{
  u_int8_t type[3];
  u_int8_t value;
};

void SendForwardingRequest(int routingSocket, u_int8_t mip);
void sendRoutingSdu(int socket_fd, char* payload, int len, u_int8_t dst_mip);
void sendHandshake(u_int8_t value);
#endif
