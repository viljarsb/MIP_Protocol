#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "socketFunctions.h"
#include "epollFunctions.h"
#include "applicationFunctions.h"

//Runs a ping server.
int main(int argc, char* argv[])
{
  char* domainPath;

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

  int unix_socket;
  unix_socket = createDomainClientSocket(domainPath);
  u_int8_t identify = 0x02;
  write(unix_socket, &identify, sizeof(u_int8_t));

  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1)
  {
     perror("epoll_create");
     return -1;
   }

  struct epoll_event events[1];
  int amountOfEntries;
  addEpollEntry(unix_socket, epoll_fd);

  //Loop forever.
  while(1)
  {
    //block until the file descriptor/socket has data to be processed.
    amountOfEntries = epoll_wait(epoll_fd, events, 1, -1);
     for (int i = 0; i < amountOfEntries; i++)
     {
       if (events[i].events & EPOLLIN)
       {
          if(events[i].data.fd == unix_socket)
         {
           //Read and print the msg, and send back a pong response.
           applicationMsg msg = readApplicationMsg(unix_socket);
           printf("MSG: %s\n", msg.payload);
           char* temp = calloc(strlen(msg.payload) + 1, sizeof(char));
           strcat(temp, "PONG ");
           strcat(temp, &msg.payload[5]);
           sendApplicationMsg(unix_socket, msg.address, temp);
           free(temp);
         }
       }
     }
  }
}
