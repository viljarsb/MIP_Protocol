#include "msgQ.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

msgQ* createQ()
{
  msgQ* q = malloc(sizeof(struct msgQ));
  q -> amountOfEntries = 0;
  q -> head = NULL;
  return q;
}

void push(msgQ* q, void* entry)
{
  if(q -> amountOfEntries + 1 > MAX_ELEMENTS)
  {
    return;
  }
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

void* pop(msgQ* q)
{
  qNode* node = q -> head;
  q -> head = q -> head -> next_node;
  q -> amountOfEntries = q -> amountOfEntries - 1;
  void* data = node -> data;
  return data;
}
