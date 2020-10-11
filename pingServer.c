#include <stdio.h> //printf.
#include <string.h> //strcat etc.
#include <sys/epoll.h> //epoll.
#include <unistd.h> //write REMOVE REMOVE REMOVE REMOVE.
#include <stdbool.h> //boolean values.

#include "socketFunctions.h" //create and connect to socket.
#include "epollFunctions.h" //Epoll utility functions.
#include "applicationFunctions.h" //sendApplicationMsg
#include "protocol.h"
#include "log.h"  //timestamp.
bool debug = true; //Implicitly true every time.

//Runs a ping server.
int main(int argc, char* argv[])
{
  //Variable to hold user specified path.
  //Create a socket variable.
  int ping_socket;

  //Very simple argument control.
  if(argc == 2 && strcmp(argv[1], "-h") == 0)
  {
    printf("Run program with <domain path>\n");
    exit(1);
  }

  else if(argc == 2)
  {
    ping_socket = createDomainClientSocket(argv[1]);
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

  sendHandshake(ping_socket, PING);

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
       printf("MIP-DEAMON SHUTDOWN -- TERMINATING EXECUTION\n");
       exit(EXIT_SUCCESS);
      }

       //Data to be read on socket.
      else if(events[i].events & EPOLLIN)
      {
        if(events[i].data.fd == ping_socket)
        {
         //Read and print the msg, and send back a pong response.
         applicationMsg* msg = calloc(1, sizeof(applicationMsg));
         bytes = readApplicationMsg(ping_socket, msg);

         if(debug)
         {
           timestamp();
           printf("RECIEVIED PING FROM: %u\n", msg -> address);
         }

         timestamp();
         printf("MSG: %s\n\n", msg -> payload);

         char* temp = calloc(bytes - 1, sizeof(char));
         strcat(temp, "PONG ");
         strcat(temp, &msg -> payload[5]);
         if(debug)
         {
           timestamp();
           printf("REPLYING TO %u\n", msg -> address);
         }
         sendApplicationMsg(ping_socket, msg -> address, temp, 0, bytes - 2);

         free(msg);
         free(temp);
         if(debug)
         {
           timestamp();
           printf("LISTENING FOR NEW PINGS\n\n");
         }
       }
      }
    }
  }
}
