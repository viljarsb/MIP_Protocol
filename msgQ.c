#include "msgQ.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

msgQ* createQ()
{
  msgQ* q = malloc(sizeof(struct msgQ));
  q -> amountOfEntries = 0;
  q -> head = NULL;
  q -> tail = NULL;
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
    struct qNode* tempNode = q -> head;
    while(tempNode -> next_node != NULL)
    {
      tempNode = tempNode -> next_node;
    }

    tempNode -> next_node = node;
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