#include <stdio.h> //printf
#include <unistd.h> //read write REMOVE REMOVE REMOVE.
#include <sys/epoll.h> //epoll
#include <string.h> //memcpy etc.
#include <stdbool.h> //boolean values.
#include <time.h> //time.
#include <sys/timerfd.h> //time_filedescriptor.
#include <signal.h> //Signals.

#include "epollFunctions.h" //Epoll utility functions.
#include "socketFunctions.h" //Socket utility functions.
#include "linkedList.h" //linkedList
#include "protocol.h" //Some definitions and declarations.
#include "applicationFunctions.h" //Send and read application msgs.
#include "routingTable.h" //Routing utility.
#include "routing_deamon.h" //Signatures of this source file.
#include "log.h" //timestamp.

//The routingtable, could be linkedList, but array allows fast lookup.
//Does not take that much space either.
routingEntry* routingTable[255];

//Som msg-codes, to send and identify incoming msgs.
u_int8_t REQUEST[3] = REQ;
u_int8_t RESPONSE[3] = RSP;
u_int8_t UPDATE[3] = UPD;
u_int8_t HELLO[3] = HEL;

u_int8_t MY_MIP_ADDRESS; //The mip-address of this node, recived from the mip-deamon.
int routingSocket; //The socket used to talk to mip-deamon.
bool debug = false; //A debug flag.
list* timeList; //A linkedlist of neighbours with a value of the last time they sent a keep-alive.

/*
    This function is used to tell neighbours that this node is active.
*/
void sendHello()
{
  if(debug)
  {
    timestamp();
    printf("SENDING HELLO-MSG TO NEIGHBOURS OVER 255\n");
  }

  helloMsg msg;
  memcpy(&msg.type, HELLO, sizeof(HELLO));

  char* buffer = calloc(1, sizeof(helloMsg));
  memcpy(buffer, &msg, sizeof(HELLO));
  sendApplicationMsg(routingSocket, MY_MIP_ADDRESS, buffer, 1, sizeof(helloMsg));
  free(buffer);
}

/*
    This function sends the entire routing table to every neighbour.
*/
void sendUpdate()
{
  if(debug)
  {
    timestamp();
    printf("SENDING UPDATE-MSG TO NEIGHBOURS\n");
  }

  //Fill in the type.
  updateStructure updateStructure;
  memcpy(updateStructure.type, UPDATE, sizeof(UPDATE));
  //Currently 0 data is in the struct.
  updateStructure.amount = 0;
  updateStructure.data = malloc(0);

  char* buffer = calloc(1, sizeof(UPDATE));
  memcpy(buffer, updateStructure.type, sizeof(UPDATE));

  //For every i in 255, check if the routing table has a entry for that mip.
  for(int i = 0; i < 255; i++)
  {
    //If there is a entry at this index (corresponding with mip)
    //Reallocate more memory, and add it to the data pointer and increase the counter by 1.
    if(findEntry(i) != NULL)
    {
      updateStructure.amount = updateStructure.amount + 1;
      updateStructure.data = realloc(updateStructure.data, sizeof(routingEntry) * updateStructure.amount);
      memcpy(updateStructure.data + (updateStructure.amount -1) * sizeof(routingEntry), &*routingTable[i], sizeof( routingEntry));
    }
  }

  //Allocate more space in the buffer. 3 bytes for type, 1 for the amount and x * sizeof a routingEntry.
  buffer = realloc(buffer, sizeof(u_int8_t) * 4 + (sizeof(routingEntry) * updateStructure.amount));

  //Copy into buffer, and send the data.
  memcpy(buffer + 3, &updateStructure.amount, sizeof(u_int8_t));
  memcpy(buffer + 4, updateStructure.data, sizeof(routingEntry) * updateStructure.amount);
  sendApplicationMsg(routingSocket, MY_MIP_ADDRESS, buffer, 1, 4 + (updateStructure.amount * sizeof(routingEntry)));
  free(updateStructure.data);
  free(buffer);
}

/*
    This function is used to reply to the mip-deamon whenever it asks for a path.

    @Param  The destination the mip-deamon wants to reach and the next_hop found.
*/
void sendResponse(int destination, u_int8_t next_hop)
{
  if(debug)
  {
    timestamp();
    printf("SENDING ROUTING-RESPONSE FOR MIP: %u -- NEXT HOP: %u\n", destination, next_hop);
  }

  routingQuery query;
  memcpy(query.type, RESPONSE, sizeof(RESPONSE));
  query.mip = next_hop;

  char* buffer = calloc(1, sizeof(routingQuery));
  memcpy(buffer, &query, sizeof(routingQuery));
  sendApplicationMsg(routingSocket, destination, buffer, -1, sizeof(routingQuery));
}

/*
    This function is called whenever the timeout runs out.
    The funtion first sends a keep-alive to all neighbours.
    Then it calls a control functions too see if some neighbours
    have failed to send their keep-alives.
*/
void alarm_()
{
  if(debug)
  {
    timestamp();
    printf("SENDING KEEP-ALIVE -- INFORMING NEIGHBOURS OF MY PRESENCE\n");
  }
  sendHello();
  controlTime();
}

/*
    This function is called whenever there is data to be read from the socket.
    The function checks what kind of data that has arrived, and determines
    what to do next.
*/
void handleIncomingMsg()
{
  applicationMsg* applicationMsg = calloc(1, sizeof(struct applicationMsg));
  int rc = readApplicationMsg(routingSocket, applicationMsg);

  //If we got a request for a pathing from the mip-deamon.
  if(memcmp(REQUEST, applicationMsg -> payload, sizeof(REQUEST)) == 0)
  {
    routingQuery query;
    memcpy(&query, applicationMsg -> payload, sizeof(routingQuery));
    if(debug)
    {
      timestamp();
      printf("RECIEVIED ROUTING-REQUEST FOR DESTINATION %u\n\n", query.mip);
    }
    u_int8_t next_hop = findNextHop(query.mip);
    sendResponse(query.mip, next_hop);
  }

  //If we got a HELLO msg from a neighbour.
  else if(memcmp(HELLO, applicationMsg -> payload, sizeof(HELLO)) == 0)
  {
    if(findEntry(applicationMsg -> address) == NULL)
    {
      if(debug)
      {
        timestamp();
        printf("RECIEVIED HELLO-BROADCAST FROM MIP: %u -- ADDING TO ROUTINGTABLE\n", applicationMsg -> address);
      }
      addToRoutingTable(applicationMsg -> address, 1, applicationMsg -> address);
      sendUpdate();
    }

    else if(findEntry(applicationMsg -> address) != NULL && findEntry(applicationMsg -> address) -> cost == 255)
    {
      updateRoutingEntry(applicationMsg -> address, 1, applicationMsg -> address);
      sendUpdate();
    }

    else
    {
      if(debug)
      {
        timestamp();
        printf("RECIVED KEEP-ALIVE BROADCAST FROM: %u\n\n", applicationMsg -> address);
      }
      updateTime(applicationMsg -> address);
    }
  }

  //If we got a UPDATE from a neighbour.
  else if(memcmp(UPDATE, applicationMsg -> payload, sizeof(UPDATE)) == 0)
  {

    if(debug)
    {
      timestamp();
      printf("RECIEVIED ROUTING-UPDATE -- UPDATING ROUTINGTABLE\n");
    }

    bool changed = false;

    updateStructure updateStructure;
    memcpy(&updateStructure, applicationMsg -> payload, sizeof(u_int8_t) * 4);
    updateStructure.data = malloc(updateStructure.amount * sizeof(routingEntry));
    memcpy(updateStructure.data, applicationMsg -> payload + 4, updateStructure.amount * sizeof(routingEntry));

    int currentPos = 0;
    int counter = 0;
    routingEntry entry;
    while(counter < updateStructure.amount)
    {
      memcpy(&entry, updateStructure.data + currentPos, sizeof(routingEntry));
      currentPos = currentPos + sizeof(routingEntry);

      if(findEntry(entry.mip_address) == NULL)
      {
        addToRoutingTable(entry.mip_address, entry.cost + 1, applicationMsg -> address);
        changed = true;
      }

      else if(findEntry(entry.mip_address) != NULL)
      {
        if(entry.cost != 255 && entry.cost + 1 < findEntry(entry.mip_address) -> cost)
        {
          updateRoutingEntry(entry.mip_address, entry.cost + 1, applicationMsg -> address);
          changed = true;
          if(debug)
          {
            timestamp();
            printf("UPDATED NEXT-HOP OF %u\n\n", entry.mip_address);
          }
        }

        else if(entry.cost == 255 && applicationMsg -> address == findEntry(entry.mip_address) -> next_hop)
        {
          updateRoutingEntry(entry.mip_address, 255, applicationMsg -> address);
          changed = true;
          if(debug)
          {
            timestamp();
            printf("PATH TO %d SET TO INFINITE\n\n", entry.mip_address);
          }
        }
      }

      counter = counter +1;
    }

    if(changed)
    {
      if(debug)
      {
        timestamp();
        printf("UPDATED COMPLETE -- TABLE WAS ALTERED -- SENDING UPDATE TO NEIGHBOURS\n");
        printRoutingTable();
      }
      sendUpdate();
    }

    else
    {
      if(debug)
      {
        timestamp();
        printf("UPDATED COMPLETE -- TABLE WAS NOT ALTERED\n\n");
      }
    }

    free(updateStructure.data);
  }

  free(applicationMsg);
}

void handle_sigint(int sig)
{
  if(debug)
  {
    timestamp();
    printf("ROUTING-DEAMON FORCE-QUIT\n");
  }

  for(int i = 0; i < 255; i++)
  {
    if(routingTable[i] != NULL)
    {
        free(routingTable[i]);
    }
  }

  freeListMemory(timeList);

  exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
  char* domainPath;
  timeList = createLinkedList(sizeof(struct timerEntry));

  if(argc == 2 && strcmp(argv[1], "-h") == 0)
  {
    printf("Run program with -d (optinal debugmode) <domain path>\n");
    exit(EXIT_SUCCESS);
  }

  else if(argc == 3 && strcmp(argv[1], "-d") == 0)
  {
    debug = true;
    domainPath = argv[2];
  }

  else
  {
    printf("Run program with -h for instructions\n");
    exit(EXIT_SUCCESS);
  }


  if(debug)
  {
    timestamp();
    printf("ROUTING_DEAMON STARTED\n\n");
  }

  routingSocket = createDomainClientSocket(domainPath);
  u_int8_t temp = ROUTING;
  write(routingSocket, &temp, sizeof(u_int8_t));
  read(routingSocket, &MY_MIP_ADDRESS, sizeof(u_int8_t));

  addToRoutingTable(MY_MIP_ADDRESS, 0, MY_MIP_ADDRESS);

  if(debug)
  {
    timestamp();
    printf("SENDING INITIAL HELLO-BROADCAST\n");
  }

  sendHello();

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
       printf("epoll_create\n");
       exit(EXIT_FAILURE);
     }

     int timer_fd;
     struct itimerspec ts;
     memset(&ts, 0, sizeof(struct itimerspec));
     ts.it_value.tv_sec = 5;
     ts.it_value.tv_nsec = 0;
     timer_fd = timerfd_create(CLOCK_REALTIME, 0);

     if(timer_fd == -1)
     {
       printf("timerfd_create");
       exit(EXIT_FAILURE);
     }

     if(timerfd_settime(timer_fd, 0, &ts, NULL) < 0)
     {
       printf("timerfd_settime");
       exit(EXIT_FAILURE);
     }

    struct epoll_event events[1];
    int amountOfEntries;
    addEpollEntry(routingSocket, epoll_fd);
    addEpollEntry(timer_fd, epoll_fd);

    //Loop forever.
    while(true)
    {
      //block until the file descriptor/socket has data to be processed.
      amountOfEntries = epoll_wait(epoll_fd, events, 1, -1);
      signal(SIGINT, handle_sigint);
      for(int i = 0; i < amountOfEntries; i++)
      {
        //Socket closed
        if(events[i].events & EPOLLHUP)
        {
          timestamp();
          printf("MIP-DEAMON SHUTDOWN -- TERMINATING EXECUTION\n");
          exit(EXIT_SUCCESS);
        }

        else if(events[i].events & EPOLLIN)
        {
            if(events[i].data.fd == routingSocket)
           {
             handleIncomingMsg();
           }

           else if(events[i].data.fd == timer_fd)
           {
             alarm_();
             timerfd_settime(timer_fd, 0, &ts, NULL);
           }
         }
      }
    }
}
