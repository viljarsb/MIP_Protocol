#ifndef LinkedList
#define LinkedList

typedef struct linkedList list;
typedef struct node node;

//Struct to store the size of the list, the size of each element and the start of the list.
struct linkedList
{
  int entries;
  int entrySize;
  struct node* head;
};

//Struct to store entries in a linkedlist, contains data pointer and a pointer to the next node.
struct node
{
  void* data;
  struct node* next;
};

list* createLinkedList(int entrySize);
void addEntry(struct linkedList* list, void* entry);
void freeListMemory(struct linkedList* list);

#endif
