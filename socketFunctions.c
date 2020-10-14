#include "socketFunctions.h" //Signatures of this file.
#include <stdio.h> //printf.
#include <sys/un.h> //sockaddr.
#include <unistd.h> //unlink.
#include <stdbool.h> //Boolean values.
#include <arpa/inet.h> //SOCK-RAW, AFPACKET etc..
#include "log.h" //timestamp.
#include "protocol.h" //Ethertype.

extern bool debug; //Extern debug flag.


/*
    This file contains functionality to create both server and client sockets.
*/



/*
    This function create a domain-socket on the server side, and sets it in
    listen-state to monitor on detect incoming connections.

    @Param  The domain-path to create the socket on.
    @Return  The socket created.
*/
int createDomainServerSocket(char* domain_path)
{
    int unix_socket;

    unix_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (unix_socket < 0)
    {
        printf("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un unix_addr;
    memset(&unix_addr, 0, sizeof(struct sockaddr_un));
    unix_addr.sun_family = AF_UNIX;
    strncpy(unix_addr.sun_path, domain_path, sizeof(unix_addr.sun_path) - 1);
    unlink(domain_path);

    if (bind(unix_socket, (const struct sockaddr*) &unix_addr, sizeof(struct sockaddr_un)) == -1) {
        printf("bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(unix_socket, MAX_APPLICATIONS) == -1) {
        printf("listen\n");
        exit(EXIT_FAILURE);
    }

    if(debug)
    {
      timestamp();
      printf("CREATED DOMAIN-SERVER-SOCKET -- LISTENING FOR INCOMING CONNECTIONS ON PATH: %s\n\n", domain_path);
    }

    return unix_socket;
}



/*
    This function create a domain-socket on the client side, and connects to
    the server.

    @Param  The domain-path to connect to.
    @Return  The socket created and connected.
*/
int createDomainClientSocket(char* domain_path)
{
  int socket_client;
  socket_client = socket(AF_UNIX, SOCK_SEQPACKET, 0);

  struct sockaddr_un addr_unix;
  memset(&addr_unix, 0, sizeof(struct sockaddr_un));
  addr_unix.sun_family = AF_UNIX;
  strncpy(addr_unix.sun_path, domain_path, sizeof(addr_unix.sun_path) - 1);

  if (connect(socket_client, (const struct sockaddr*) &addr_unix, sizeof(struct sockaddr_un)) == -1) {
      timestamp();
      printf("connect");
      exit(EXIT_FAILURE);
  }

  if(debug)
  {
    timestamp();
    printf("CREATED DOMAIN-CLIENT-SOCKET -- CONNECTED TO SERVER-SOCKET ON PATH: %s\n\n", domain_path);
  }

  return socket_client;
}



/*
    This function create a raw-socket, of type AF_PACKET and type ETH_P_MIP.

    @Return  The raw-socket.
*/
int createRawSocket()
{
  int sock_fd;

    sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_MIP));
    if (sock_fd == -1) {
        timestamp();
        printf("raw-socket\n");
        exit(EXIT_FAILURE);
    }

    if(debug)
    {
      timestamp();
      printf("CREATED RAW-SOCKET\n\n");
    }

    return sock_fd;
}
