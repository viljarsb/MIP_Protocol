#include <stdio.h>
#include "socketFunctions.h"
#include "epollFunctions.h"
#include "applicationFunctions.h"
#include <sys/select.h>
#include <time.h>

int main(int argc, char* argv[])
{
  u_int8_t dst_addr;
  char* msg;
  char* domainPath;

  if(argc == 2 && strcmp(argv[1], "-h") == 0)
  {
    printf("Run program with <Destination host> <msg> <domain path>\n");
    exit(1);
  }

  else if(argc == 4)
  {
    dst_addr = atoi(argv[1]);
    msg = malloc(strlen(argv[2]) + 1);
    strcpy(msg, argv[2]);
    domainPath = malloc(strlen(argv[3]) + 1);
    strcpy(domainPath, argv[3]);
  }

  else if(argc != 4)
  {
    printf("Program can not run.\nAttempt to run with -h for instructions.\n");
    exit(1);
  }

  int ping_socket;
  int rc;
  fd_set set;
  clock_t stopwatch;

  ping_socket = createDomainClientSocket(domainPath);
  sendApplicationMsg(ping_socket, dst_addr, msg);
  stopwatch = clock();

  FD_ZERO(&set);
  FD_SET(ping_socket, &set);
  struct timeval tv = {1, 0};

  rc = select(FD_SETSIZE, &set, NULL, NULL, &tv);

  if(rc && FD_ISSET(ping_socket, &set))
  {
    applicationMsg msg = readApplicationMsg(ping_socket);

    printf("Recieved response in %f seconds\n", (double)(clock() - stopwatch)/1000);
  }

  else
  {
      printf("Client did not recieve any PONG respone.\n Timing out.");
  }
  close(ping_socket);
}
