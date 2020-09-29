#ifndef MSGQ
#define MSGQ

#define MAX_ELEMENTS 15
typedef struct qNode qNode;
typedef struct msgQ msgQ;

struct qNode
{
    qNode* next_node;
    int size;
    void* data;
};

struct msgQ
{
  qNode* head;
  qNode* tail;
  int amountOfEntries;
};

msgQ* createQ();
void push(struct msgQ* q, void* entry, int size);
qNode* pop(struct msgQ* q);

#endif
