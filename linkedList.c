#include "linkedList.h"
#include <stdlib.h> //free.
#include <string.h> //memcpy.

/*
    This file contains functions to create a generic linked list.
    However not all functionlaity is implemented, if needed it must
    be implemented in other parts of the program.
*/



/*
    This function create a linkedList.

    @Param  The size of the entry elements.
    @Return  A pointer to a linedList struct, this struct contains a pointer to the first node, and some info such as amount of entries.
*/
list* createLinkedList(int entrySize)
{
  list* list = malloc(sizeof(struct linkedList));
  list -> entries = 0;
  list -> entrySize = entrySize;
  list -> head = NULL;
  return list;
}

/*
    This function adds some data into a specified linkedList.

    @Param  The linkedList to add into, and a pointer to the entry.
*/
void addEntry(list* list, void* entry)
{
  node* node = malloc(sizeof(struct node));
  node -> data = malloc(list -> entrySize);
  memcpy(node -> data, entry, list -> entrySize);
  node -> next = NULL;

  //If head empty, insert as first in the list.
  if(list -> head == NULL)
  {
    list -> head = node;
  }

  //If not traverse the list until a empty spot is found.
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

/*
    This funtions frees the allocated space for nodes and the list.

    @Param  The list to free.
*/
void freeListMemory(list* list)
{
  node* tempNode = list -> head;

  while(tempNode != NULL)
  {
    node* nextNode = tempNode;
    tempNode = tempNode -> next;
    free(nextNode -> data);
    free(nextNode);
  }

  free(list);
}
