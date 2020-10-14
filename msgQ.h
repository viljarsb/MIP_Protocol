#ifndef MSGQ
#define MSGQ
#include "protocol.h" //mip_header.

#define MAX_ELEMENTS 15
typedef struct qNode qNode;
typedef struct msgQ msgQ;
typedef struct unsent unsent;
//A nnode of the Q, has a pointer to next node and pointer to data.
struct qNode
{
    qNode* next_node;
    void* data;
};


//The Q itself, has a head and a amount of entires value.
struct msgQ
{
  qNode* head;
  int amountOfEntries;
};

struct unsent
{
  mip_header* mip_header;
  char* buffer;
};


msgQ* createQ();
void push(struct msgQ* q, void* entry);
void* pop(struct msgQ* q);

#endif
