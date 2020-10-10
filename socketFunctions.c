#include "socketFunctions.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "log.h"
#include "protocol.h"
extern bool debug;

int createDomainServerSocket(char* domain_path)
{
    int unix_socket;

    unix_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (unix_socket < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un unix_addr;
    memset(&unix_addr, 0, sizeof(struct sockaddr_un));
    unix_addr.sun_family = AF_UNIX;
    strncpy(unix_addr.sun_path, domain_path, sizeof(unix_addr.sun_path) - 1);
    unlink(domain_path);

    if (bind(unix_socket, (const struct sockaddr*) &unix_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(unix_socket, MAX_APPLICATIONS) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if(debug)
    {
      timestamp();
      printf("CREATED DOMAIN-SERVER-SOCKET -- LISTENING FOR INCOMING CONNECTIONS ON PATH: %s\n\n", domain_path);
    }

    return unix_socket;
}

int createDomainClientSocket(char* domain_path)
{
  int socket_client;
  socket_client = socket(AF_UNIX, SOCK_SEQPACKET, 0);

  struct sockaddr_un addr_unix;
  memset(&addr_unix, 0, sizeof(struct sockaddr_un));
  addr_unix.sun_family = AF_UNIX;
  strncpy(addr_unix.sun_path, domain_path, sizeof(addr_unix.sun_path) - 1);

  if (connect(socket_client, (const struct sockaddr*) &addr_unix, sizeof(struct sockaddr_un)) == -1) {
      perror("connect");
      return -1;
  }

  if(debug)
  {
    timestamp();
    printf("CREATED DOMAIN-CLIENT-SOCKET -- CONNECTED TO SERVER-SOCKET ON PATH: %s\n\n", domain_path);
  }

  return socket_client;
}

int createRawSocket()
{
  int sock_fd;

    sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_MIP));
    if (sock_fd == -1) {
        perror("socket");
        return -1;
    }

    if(debug)
    {
      timestamp();
      printf("CREATED RAW-SOCKET\n\n");
    }

    return sock_fd;
}
