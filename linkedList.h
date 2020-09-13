#ifndef LinkedList
#define LinkedList
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct linkedList list;
typedef struct node node;

struct linkedList
{
  int entries;
  int entrySize;
  struct node* head;
};

struct node
{
  void* data;
  struct node* next;
};

list* createLinkedList(int entrySize);
void addEntry(struct linkedList* list, void* entry);
void freeListMemory(struct linkedList* list);

#endif