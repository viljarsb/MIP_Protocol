#include <stdio.h> //printf.
#include <string.h> //strcat etc.
#include <sys/epoll.h> //epoll.
#include <unistd.h> //close
#include <stdbool.h> //Boolean values.
#include <signal.h> //signals.
#include "socketFunctions.h" //create and connect to socket.
#include "epollFunctions.h" //Epoll utility functions.
#include "applicationFunctions.h" //sendApplicationMsg
#include "log.h"  //timestamp.
bool debug = true; //Implicitly true every time.
#define PING 0x02

//simple signalhandler.
void handle_sigint(int sig)
{
  timestamp();
  printf("PING-SERVER FORCEQUIT\n");
  exit(EXIT_SUCCESS);
}


//Runs a ping server.
int main(int argc, char* argv[])
{
  //Create a socket variable.
  int ping_socket;

  //Very simple argument control.
  if(argc == 2 && strcmp(argv[1], "-h") == 0)
  {
    printf("Run program with <domain path>\n");
    exit(EXIT_SUCCESS);
  }

  else if(argc == 2)
  {
    ping_socket = createDomainClientSocket(argv[1]);
  }

  else if(argc != 2)
  {
    printf("Program can not run.\nAttempt to run with -h for instructions.\n");
    exit(EXIT_SUCCESS);
  }

  if(debug)
  {
    timestamp();
    printf("PING-SERVER RUNNING -- LISTENING FOR PINGS\n\n");
  }

  //Identify this program.
  sendHandshake(ping_socket, PING);

  //Create epoll-set
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
  while(true)
  {
    signal(SIGINT, handle_sigint);
    //block until the socket-fd has data to be processed.
    amountOfEntries = epoll_wait(epoll_fd, events, 1, -1);
    for(int i = 0; i < amountOfEntries; i++)
    {
      //Socket closed
      if(events[i].events & EPOLLHUP)
      {
       timestamp();
       printf("MIP-DEAMON SHUTDOWN -- TERMINATING EXECUTION\n");
       close(ping_socket);
       close(epoll_fd);
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
