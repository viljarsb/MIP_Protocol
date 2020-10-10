#include "msgQ.h"

/*
    This function create a empty Q.
    @Return  a pointer to a msgQ, used to access the Q.
*/
msgQ* createQ()
{
  msgQ* q = malloc(sizeof(struct msgQ));
  q -> amountOfEntries = 0;
  q -> head = NULL;
  return q;
}

/*
    This function pushes (adds to the tail) of the Q.

    @Param  The q to add into, a pointer to the data to add.
*/
void push(msgQ* q, void* entry)
{
  //If q is full, return.
  if(q -> amountOfEntries + 1 > MAX_ELEMENTS)
  {
    return;
  }

  //Else create structures and find the last spot in the Q and add the entry.
  qNode* node = malloc(sizeof(struct qNode));
  node -> next_node = NULL;
  node -> data = entry;
  if(q -> head == NULL)
  {
    q -> head = node;
  }

  else
  {
    node = q -> head;
    while(node -> next_node != NULL)
    {
      node = node -> next_node;
    }

    node -> next_node = node;
  }

  q -> amountOfEntries = q -> amountOfEntries + 1;
}

/*
    This function pops (fetches the head) of a q.

    @Param the Q to pop.
    @Return a void pointer to the data of the popped entry.
*/
void* pop(msgQ* q)
{
  qNode* node = q -> head;
  q -> head = q -> head -> next_node;
  q -> amountOfEntries = q -> amountOfEntries - 1;
  void* data = node -> data;
  free(node);
  return data;
}
