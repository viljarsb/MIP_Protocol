#ifndef MSGQ
#define MSGQ
#include "applicationFunctions.h"

#define MAX_ELEMENTS 15
typedef struct qNode qNode;
typedef struct msgQ msgQ;

struct qNode
{
    qNode* next_node;
    void* data;
};

struct waitingMsg{
  int addr;
  applicationMsg* appMsg;
};

struct msgQ
{
  qNode* head;
  qNode* tail;
  int amountOfEntries;
};

msgQ* createQ();
void push(struct msgQ* q, void* entry);
void* pop(struct msgQ* q);

#endif
