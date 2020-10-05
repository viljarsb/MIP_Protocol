#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "socketFunctions.h"
#include "applicationFunctions.h"
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
bool debug = false;

int main(int argc, char* argv[])
{
  u_int8_t dst_addr;
  char* msg;
  u_int8_t ttl;
  char* domainPath;

  if(argc == 2 && strcmp(argv[1], "-h") == 0)
  {
    printf("Run program with <Destination host> <msg> <ttl> <domain path>\n");
    exit(1);
  }

  else if(argc == 5)
  {
    dst_addr = atoi(argv[1]);
    msg = calloc(strlen(argv[2]) + 6, sizeof(char));
    strcat(msg, "PING ");
    strcat(msg, argv[2]);
    ttl = atoi(argv[3]);
    if(ttl < 0)
    {
      printf("Please supply a zero (undefined) or another positive ttl");
      exit(1);
    }
    domainPath = argv[4];
  }

  else if(argc != 5)
  {
    printf("Program can not run.\nAttempt to run with -h for instructions.\n");
    exit(1);
  }

  int ping_socket;
  int rc;
  fd_set set;
  clock_t stopwatch;

  ping_socket = createDomainClientSocket(domainPath);
  u_int8_t identify = 0x02;
  write(ping_socket, &identify, sizeof(u_int8_t));

  sendApplicationMsg(ping_socket, dst_addr, msg, ttl, strlen(msg));
  stopwatch = clock();

  FD_ZERO(&set);
  FD_SET(ping_socket, &set);
  struct timeval tv = {1, 0};

  rc = select(FD_SETSIZE, &set, NULL, NULL, &tv);

  if(rc && FD_ISSET(ping_socket, &set))
  {
    applicationMsg* msg = calloc(1, sizeof(applicationMsg));
    readApplicationMsg(ping_socket, msg);

    printf("MSG: %s\n", msg -> payload);
    printf("Recieved response in %f seconds\n", (double)(clock() - stopwatch)/1000);
  }

  else
  {
      printf("Client did not recieve any PONG respone.\nTiming out.");
  }

  free(msg);
  close(ping_socket);
}
