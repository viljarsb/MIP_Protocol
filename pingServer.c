#include <stdio.h> //printf.
#include <string.h> //strcat etc.
#include <sys/epoll.h> //epoll.
#include <unistd.h> //write REMOVE REMOVE REMOVE REMOVE.
#include <stdbool.h> //boolean values.

#include "socketFunctions.h" //create and connect to socket.
#include "epollFunctions.h" //Epoll utility functions.
#include "applicationFunctions.h" //sendApplicationMsg
#include "log.h"  //timestamp.
bool debug = true; //Implicitly true every time.

//Runs a ping server.
int main(int argc, char* argv[])
{
  //Variable to hold user specified path.
  char* domainPath;

  //Very simple argument control.
  if(argc == 2 && strcmp(argv[1], "-h") == 0)
  {
    printf("Run program with <domain path>\n");
    exit(1);
  }

  else if(argc == 2)
  {
    domainPath = malloc(strlen(argv[1]) + 1);
    strcpy(domainPath, argv[1]);
  }

  else if(argc != 2)
  {
    printf("Program can not run.\nAttempt to run with -h for instructions.\n");
    exit(1);
  }

  if(debug)
  {
    timestamp();
    printf("PING-SERVER RUNNING -- LISTENING FOR PINGS\n\n");
  }

  //Create a socket and connect (done trough function in socketFunctions.h).
  int ping_socket;
  ping_socket = createDomainClientSocket(domainPath);
  //identify.
  u_int8_t identify = 0x02;
  write(ping_socket, &identify, sizeof(u_int8_t));

  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1)
  {
     printf("epoll_create\n");
     exit(EXIT_FAILURE);
   }

  struct epoll_event events[1];
  int amountOfEntries;
  addEpollEntry(ping_socket, epoll_fd);
  int bytes;

  //Loop forever.
  while(1)
  {
    //block until the socket-fd has data to be processed.
    amountOfEntries = epoll_wait(epoll_fd, events, 1, -1);
    for(int i = 0; i < amountOfEntries; i++)
    {
      //Socket closed
      if(events[i].events & EPOLLHUP)
      {
       timestamp();
       printf("\n\nMIP-DEAMON SHUTDOWN -- TERMINATING EXECUTION\n");
       exit(EXIT_SUCCESS);
      }

       //Data to be read on socket.
      if(events[i].events & EPOLLIN)
      {
        if(events[i].data.fd == ping_socket)
        {
         //Read and print the msg, and send back a pong response.
         applicationMsg* msg = calloc(1, sizeof(applicationMsg));
         readApplicationMsg(ping_socket, msg);
         timestamp();
         printf("RECIEVIED PING FROM: %u\n", msg -> address);
         timestamp();
         printf("MSG: %s\n\n", msg -> payload);

         char* temp = calloc(bytes - 2, sizeof(char));
         strcat(temp, "PONG ");
         strcat(temp, &msg -> payload[5]);
         sendApplicationMsg(ping_socket, msg -> address, temp, 0, bytes - 2);
         free(temp);

          timestamp();
          printf("LISTENING FOR NEW PINGS\n\n");
       }
      }
    }
  }
}
