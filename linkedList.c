#include "linkedList.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

list* createLinkedList(int entrySize)
{
  list* list = malloc(sizeof(struct linkedList));
  list -> entries = 0;
  list -> entrySize = entrySize;
  list -> head = NULL;
  return list;
}

void addEntry(list* list, void* entry)
{
  node* node = malloc(sizeof(struct node));
  node -> data = malloc(list -> entrySize);
  memcpy(node -> data, entry, list -> entrySize);
  node -> next = NULL;

  if(list -> head == NULL)
  {
    list -> head = node;
  }

  else
  {
    struct node* tempNode = list -> head;
    while(tempNode -> next != NULL)
    {
      tempNode = tempNode -> next;
    }

    tempNode -> next = node;
  }

  list -> entries = list -> entries + 1;
}

void freeListMemory(list* list)
{
  node* tempNode = list -> head;

  while(tempNode != NULL)
  {
    node* nextNode = tempNode -> next;
    free(tempNode -> data);
    free(tempNode);
    tempNode = nextNode;
  }

  free(list);
}
