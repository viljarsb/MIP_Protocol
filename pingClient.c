#include <stdio.h> //printf
#include <string.h> //strlen, memcpy.
#include <time.h> //timeout.
#include <stdbool.h> //Boolean values.
#include <unistd.h> //close.
#include "socketFunctions.h"  //create and connect socket.
#include "applicationFunctions.h" //sendApplicationMsg.
#include "log.h"  //timestamp.
bool debug = false; //included just because other parts of program needs it. Has no function here.

#define PING 0x02

int main(int argc, char* argv[])
{
  //Some variables to holder user arguments.
  u_int8_t dst_addr;
  char* msg;
  u_int8_t ttl;
  char* domainPath;

  //Some simple argument control.
  if(argc == 2 && strcmp(argv[1], "-h") == 0)
  {
    printf("Run program with <Destination host> <msg> <domain path>\n");
    exit(EXIT_SUCCESS);
  }

  else if(argc == 4)
  {
    dst_addr = atoi(argv[1]);
    msg = calloc(strlen(argv[2]) + 6, sizeof(char));
    strcat(msg, "PING ");
    strcat(msg, argv[2]);
    domainPath = argv[3];
  }

  else if(argc != 4)
  {
    printf("Program can not run.\nAttempt to run with -h for instructions.\n");
    exit(EXIT_SUCCESS);
  }

  //Some variables for the socket, amount of bytes, the fd-set and the clock.
  int ping_socket;
  int rc;
  fd_set set;
  clock_t stopwatch;

  //Create and connect to the mip-deamon on path specified, done trough a function in (socketFunctions.c).
  ping_socket = createDomainClientSocket(domainPath);

  //Identify this program.
  sendHandshake(ping_socket, PING);

  //Send the msg from the client to the mip-deamon and start a timer.
  sendApplicationMsg(ping_socket, dst_addr, msg, 0, strlen(msg));
  stopwatch = clock();

  FD_ZERO(&set);
  FD_SET(ping_socket, &set);
  struct timeval tv = {1, 0};
  rc = select(FD_SETSIZE, &set, NULL, NULL, &tv);

  //If response is recived before 1 sec has passed.
  if(rc && FD_ISSET(ping_socket, &set))
  {
    applicationMsg* applicationMsg = calloc(1, sizeof(applicationMsg));
    readApplicationMsg(ping_socket, applicationMsg);
    timestamp();
    printf("MSG: %s\n", applicationMsg -> payload);
    printf("Recieved response in %f seconds\n", (double)(clock() - stopwatch)/1000);
    free(applicationMsg);
  }

  //Timer ran out.
  else
  {
    timestamp();
    printf("Client did not recieve any PONG respone.\nTiming out.\n");
  }

  free(msg);
  close(ping_socket);
}
