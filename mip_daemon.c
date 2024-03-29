#include <stdbool.h> // Boolean-type.
#include <stdio.h> // I/O.
#include <unistd.h> //Standard POSIX.1 functions, constants and types.
#include <string.h> //Tyoes, macros and functions to manipulate char-arrays.
#include <sys/epoll.h> //File monitoring.
#include <signal.h> //Handle signals.
#include <sys/socket.h> //close.

#include "mip_daemon.h" //Signatures for functions of this source file.
#include "protocol.h" //Definitions of constants and datastructures (formats) of the MIP-Protocol.
#include "applicationFunctions.h" //Send and read data from other processes.
#include "arpFunctions.h" //Arp functionality, sendResponse, sendBroadcast and store mip-datagrams waiting for responses etc..
#include "interfaceFunctions.h" //Fetch and store interfaces on this node.
#include "rawFunctions.h" //Functionality to send and reciveve raw data.
#include "socketFunctions.h" //Create sockets.
#include "msgQ.h" //Q for msgs waiting for routing-deamon to respond.
#include "epollFunctions.h" //Utility functions for EPOLL.
#include "routing.h" //Definitions and data structures.
#include "log.h" //timestamp.

//Some Routing-SDU codes.
extern const u_int8_t REQUEST[3];
extern const u_int8_t RESPONSE[3];


u_int8_t MY_MIP_ADDRESS; //This node's mip.
bool debug = false; //Debug flag
list* interfaces; //LinkedList of interfaces on this host.
list* arpCache; //LinkedList of entries in arp-cache.
msgQ* waitingQ; //A q for mip-packets waiting for route info from routing-deamon.
list* arpWaitingList; //We cant use a Q for arp-msgs waiting for mac destinations, we cant guarantee they are responded to in order.



/*
  This function is called whenever a packet arrives over the raw-socket.
  It looks at the MIP-Header to determine what kind of packet that has arrived and acts accordingly.

  @Param  The raw-socket, the ping-socket and the routing-socket.
*/
void handleRawPacket(int socket_fd, int pingApplication, int routingApplication)
{
    //Construct and allocate space for pointers to a ethernet_header, a mip_header, a int (the interfaced the packet is recived on) and a buffer for the SDU.
    ethernet_header* ethernet_header = calloc(1, sizeof(struct ethernet_header));
    mip_header* mip_header = calloc(1, sizeof(struct mip_header));
    int* recivedOnInterface = calloc(1, sizeof(int));
    char* buffer = calloc(1, 1024);

    //Read from the socket into the pointers passed as parameters. Function implemented in rawFunctions.c
    readRawPacket(socket_fd, ethernet_header, mip_header, buffer, recivedOnInterface);

    //A ARP-SDU arrived
    if(mip_header -> sdu_type == ARP)
    {
      //Construct a struct arpMsg and copy content of SDU into this struct.
      struct arpMsg arpMsg;
      memcpy(&arpMsg, buffer, sizeof(struct arpMsg));

      //If the MIP-address is 255 (broadcast) and the  arp-type is ARP-BROADCAST
       if(mip_header -> dst_addr == 0xFF && arpMsg.type == ARP_BROADCAST)
       {
         //If the sender asks for my MAC-address.
         if(arpMsg.mip_address == MY_MIP_ADDRESS)
         {
           //Check if the this node already has the requesting-node's MAC. If not add it to cache.
           if(getCacheEntry(mip_header -> src_addr) == NULL)
             addArpEntry(mip_header -> src_addr, ethernet_header -> src_addr, *recivedOnInterface);

          //Update entry.
           else
             updateArpEntry(mip_header -> src_addr, ethernet_header -> src_addr, *recivedOnInterface);

           if(debug)
           {
             timestamp();
             printf("RECEIVED ARP-REQUEST -- MIP SRC: %d -- MIP DST: %d\n", mip_header -> src_addr, mip_header -> dst_addr);
             printArpCache();
           }

           //Send a ARP-RESPONSE to the requesting-node.
           sendArpResponse(socket_fd, mip_header -> src_addr);
         }

         else
         {
           if(debug)
           {
             timestamp();
             printf("INTERCEPTED A ARP-BROADCAST MEANT FOR ANOTHER HOST -- CACHING %u ANYWAY\n", mip_header -> src_addr);
           }

           if(getCacheEntry(mip_header -> src_addr) == NULL)
             addArpEntry(mip_header -> src_addr, ethernet_header -> src_addr, *recivedOnInterface);
         }
       }

       //response
       else if(mip_header -> dst_addr == MY_MIP_ADDRESS && arpMsg.type == ARP_RESPONSE)
       {
        addArpEntry(mip_header -> src_addr, ethernet_header -> src_addr, *recivedOnInterface);

        if(debug)
        {
          timestamp();
          printf("RECEIVED ARP-RESPONSE -- SRC MIP: %d -- DST MIP: %d", mip_header -> src_addr, mip_header -> dst_addr);
          printArpCache();
        }


        //If there are waiting msgs in the arpList, check if anyone of them are ready for be sent.
        if(arpWaitingList -> entries > 0)
          sendWaitingMsgs(socket_fd, arpWaitingList, mip_header -> src_addr);
     }
    }

    //A PING-SDU arrived
    else if(mip_header -> sdu_type == PING)
    {
      //The ping is addressed to this node.
      if(mip_header -> dst_addr == MY_MIP_ADDRESS)
      {
        if(debug)
        {
          timestamp();
          printf("RECEIVED PING -- MIP SRC: %d -- MIP DST: %d", mip_header -> src_addr, mip_header -> dst_addr);
        }

        //Construct a applicationMsg struct and copy the SDU into this.
        applicationMsg msg;
        memcpy(&msg.address, &mip_header -> src_addr, sizeof(u_int8_t));
        memcpy(msg.payload, buffer, mip_header -> sdu_length);
        //If we have a pingApplication running, send the MSG to that application.
        if(pingApplication != -1)
          sendApplicationMsg(pingApplication, msg.address, msg.payload, 0, mip_header -> sdu_length);

        //if not print a error.
        else
          if(debug)
          {
            timestamp();
            printf("CAN NOT SEND DATA TO PING APPLICATION -- NO SUCH APPLICATION IS CURRENTLY RUNNING");
          }
      }

      //The ping is addressed to some other node and is routed trough this node.
      else if(mip_header -> dst_addr != MY_MIP_ADDRESS)
      {
        //Decrease the ttl and control that it has not reached 0.
        mip_header -> ttl = mip_header -> ttl -1;
        if(mip_header -> ttl <= 0)
        {
          if(debug)
          {
            timestamp();
            printf("RECEIVED PING FOR ANOHTER HOST -- TTL HAS REACHED 0 -- DISCARDING PACKET WITH DESTINATION: %u\n", mip_header -> dst_addr);
          }
        }

        else
        {
          if(debug)
          {
            timestamp();
            printf("RECEIVED PING FOR ANOHTER HOST -- TRYING TO FORWARDING TO %u FOR %u\n", mip_header -> dst_addr, mip_header -> src_addr);
          }

          //Save the msg and Q and wait for response from routing-deamon before deciding what to do with the mip-datagram.
          unsent* unsent = malloc(sizeof(struct unsent));
          unsent -> mip_header = malloc(sizeof(struct mip_header));
          unsent -> buffer = malloc(mip_header -> sdu_length);
          memcpy(unsent -> mip_header, mip_header, sizeof(struct mip_header));
          memcpy(unsent -> buffer, buffer, mip_header -> sdu_length);
          push(waitingQ, unsent);

          //Ask the routing deamon for next hop of this packet.
          sendForwardingReuest(routingApplication, unsent -> mip_header -> dst_addr);
        }
      }
    }

    //A ROUTING-SDU arrived.
    else if(mip_header -> sdu_type == ROUTING)
    {
      if(debug)
      {
        timestamp();
        printf("RECIEVIED ROUTING-SDU -- MIP SRC: %u -- MIP DEST: %u\n", mip_header -> src_addr, mip_header -> dst_addr);
      }

      //Construct a applicationMsg and copy content of SDU into this.
      applicationMsg msg;
      memset(&msg, 0, sizeof(struct applicationMsg));
      memcpy(&msg.address, &mip_header -> src_addr, sizeof(u_int8_t));
      memcpy(msg.payload, buffer, mip_header -> sdu_length);

      //Send the data to the routing-deamon if its running.
      if(routingApplication != -1)
        sendApplicationMsg(routingApplication, msg.address, msg.payload, 0, mip_header -> sdu_length);

      else
        if(debug)
        {
          timestamp();
          printf("CAN NOT SEND DATA TO ROUTINGDEAMON -- NO ROUTINGDEAMON RUNNING\n");
        }
    }

    //free allocated memory.
    free(ethernet_header);
    free(recivedOnInterface);
    free(mip_header);
    free(buffer);
  }



/*
  This function is called whenever a packet arrives over the ping-socket.
  It looks at the address in the applicationMsg struct and determines where to send the packet.
  @Param  The ping-socket, the routing-socket and the raw-socket.
*/
void handleApplicationPacket(int activeApplication, int routingApplication, int socket_fd)
{
  //Construct a applicationMsg pointer and allocate space for this. Then read from the socket into this pointer.
  applicationMsg* applicationMsg = calloc(1, sizeof(struct applicationMsg));
  int rc = readApplicationMsg(activeApplication, applicationMsg);


  //The msg from the application is addressed to this node.
  if(applicationMsg -> address == MY_MIP_ADDRESS)
  {
    if(debug)
    {
      timestamp();
      printf("A APPLICATION ON THIS HOST ADDRESS A PACKET TO IT SELF -- SENDING THE MSG IN RETURN");
    }

    //Send the msg in return
    sendApplicationMsg(activeApplication, applicationMsg -> address, applicationMsg -> payload, 0, rc - 2); //Length of payload - applicaitonMsg adr and ttl
  }

  //The ping is addressed to a remote-node
  else if(applicationMsg -> address != MY_MIP_ADDRESS)
  {
    //Ask the routing-deamon for the next hop of this msg.
    if(routingApplication != -1)
    {
        sendForwardingReuest(routingApplication, applicationMsg -> address);

        //Save the msg and q the msg, msg will be handled whenever the routing-deamon answers the request.
        struct unsent* unsent = malloc(sizeof(struct unsent));
        unsent -> mip_header = calloc(1, sizeof(struct mip_header));
        unsent -> mip_header -> src_addr = MY_MIP_ADDRESS;
        unsent -> mip_header -> dst_addr = applicationMsg -> address;
        unsent -> mip_header -> sdu_type = PING;
        if(applicationMsg -> ttl > 0)
          unsent -> mip_header -> ttl = applicationMsg -> ttl;
        else
          unsent -> mip_header -> ttl = DEFAULT_TTL_MIP
        unsent -> mip_header -> sdu_length = rc - 2;
        unsent -> buffer = calloc(1, unsent -> mip_header -> sdu_length);
        memcpy(unsent -> buffer, applicationMsg -> payload, unsent -> mip_header -> sdu_length);
        push(waitingQ, unsent);
    }

    else
    {
      printf("CAN NOT FOWARD TO REMOTE HOST -- NO ROUTINGDEAMON RUNNING");
    }
  }

  //free the msg.
  free(applicationMsg);
}



/*
  This function is called whenever a packet arrives over the routing-socket.
  It looks at the type of routingMsg and determines what to do.
  @Param  The raw-socket, the routing-socket.
*/
void handleRoutingPacket(int socket_fd, int routingApplication)
{
  //Construct a applicationMsg pointer and allocate space for this. Then read from the socket into this pointer.
  applicationMsg* applicationMsg = calloc(1, sizeof(struct applicationMsg));
  int rc = readApplicationMsg(routingApplication, applicationMsg);

  //Inspect the first 3-bytes of the payload to determine type of msg.

  //The msg recived from the routing-deamon is a RESPONSE to a REQUEST.
  if(memcmp(RESPONSE, applicationMsg -> payload, sizeof(RESPONSE)) == 0)
  {
    //Copy content of msg into a routingQuery struct.
    routingQuery query;
    memcpy(&query, &applicationMsg -> payload, sizeof(routingQuery));

    //If the response is 255, no route was found.
    if(query.mip == 255)
    {
      if(debug)
      {
        timestamp();
        printf("NO ROUTE FOUND FOR DESTINATION: %u -- DISCARDRING PACKET\n\n", applicationMsg -> address);
      }

      //pop the q, and free the memory.
      unsent* unsent = pop(waitingQ);
      free(unsent -> mip_header);
      free(unsent -> buffer);
      free(unsent);
      return;
    }

    //If response is not 255, a route has been found.
    else if(query.mip != 255)
    {
      //Pop the waitingQ.
      unsent* unsent = pop(waitingQ);

      if(debug)
      {
          timestamp();
          printf("ROUTE FOUND FOR DESTINATION: %u -- ROUTING VIA: %u\n", unsent -> mip_header -> dst_addr, query.mip);
          timestamp();
          printf("SENDING PING-SDU -- MIP SRC: %u -- MIP DST: %u\n", unsent -> mip_header -> src_addr, unsent -> mip_header -> dst_addr);
        }

      //Pass the contents of the mip_header and the application msg over to the sendData function along with the next_hop.
      sendData(socket_fd, unsent -> mip_header, unsent -> buffer, query.mip);
      free(unsent);
    }
  }

  //If its not a a response, the mip-deamon does not care what it is, just send it where it is addressed.
  else
  {
    u_int8_t dst_mip;
    memcpy(&dst_mip, &applicationMsg -> address, sizeof(u_int8_t)); //If not a response, adr for packet is at offset 3, after type.

    //Create a char pointer and allocate space for the SDU payload (recived bytes - ttl and Address of the applicationMsg)
    char* buffer = calloc(1, rc - (sizeof(u_int8_t) * 2));
    memcpy(buffer, applicationMsg -> payload, rc - (sizeof(u_int8_t) * 2)); //payload = amount of bytes read - offset of two (fields in the applicaitonMsg)

    //Send the data.
    sendRoutingSdu(socket_fd, buffer, rc - (sizeof(u_int8_t) * 2), dst_mip); //payload = amount of bytes read - offset of two (fields in the applicaitonMsg)
  }

  //free the msg.
  free(applicationMsg);
}



/*
    A very simple signal handler called whenever the user CTRL-C
*/

void handle_sigint(int sig)
{
    if(debug)
    {
      timestamp();
      printf("mip_daemon FORCE-QUIT\n");
    }

    freeInterfaces(interfaces);
    freeListMemory(arpCache);
    freeArpList(arpWaitingList);
    free(waitingQ);
    exit(EXIT_SUCCESS);
}



/*
    The main function of the mip-deamon.
*/
int main(int argc, char* argv[])
{
  char* domainPath;

  //Some very simple argument control.
  if(argc == 4 && strcmp(argv[1], "-d") == 0)
  {
    debug = true;
    domainPath = argv[2];
    MY_MIP_ADDRESS = atoi(argv[3]);
  }

  else if(argc == 2 && strcmp(argv[1], "-h") == 0)
  {
    printf("Run program with -d (optional debugmode) <domain path> <mip address>\n");
    exit(EXIT_SUCCESS);
  }

  else if(argc == 3)
  {
    domainPath = argv[1];
    MY_MIP_ADDRESS = atoi(argv[2]);
  }

  else
  {
    printf("Run program with -h as argument for instructions\n");
    exit(EXIT_SUCCESS);
  }

  if(debug)
  {
    timestamp();
    printf("MIP-DEAMON HAS STARTED RUNNING ON HOST %d\n\n", MY_MIP_ADDRESS);
  }

  //Create a linked list, and fill it with active interfaces.
  interfaces = createLinkedList(sizeof(interface));
  findInterfaces(interfaces);

  //Create the arp-cache.
  arpCache = createLinkedList(sizeof(arpEntry));

  //Create the Q's for waiting msgs.
  waitingQ = createQ();
  arpWaitingList = createLinkedList(sizeof(struct arpWaitEntry));
  //Sockets.
  int socket_fd, domain_socket;
  int pingApplication = -1;
  int routingApplication = -1;
  int tempApplication = -1;

  //Create the server sockets, both the raw and the domain-socket.
  domain_socket = createDomainServerSocket(domainPath);
  socket_fd = createRawSocket();

  //Create epoll-set and add sockets to monitor.
  int epoll_fd = epoll_create1(0);
  if(epoll_fd == -1)
  {
     printf("epoll_create\n");
     exit(EXIT_FAILURE);
  }

  struct epoll_event events[10];
  int amountOfEntries;
  addEpollEntry(domain_socket, epoll_fd);
  addEpollEntry(socket_fd, epoll_fd);

  //Loop forever.
  while(true)
  {
    signal(SIGINT, handle_sigint);
    amountOfEntries = epoll_wait(epoll_fd, events, 10, -1);
    for(int i = 0; i < amountOfEntries; i++)
    {
      //If we get data over domain-socket, find out program that connected.
      if(events[i].data.fd == domain_socket)
      {
         tempApplication = accept(domain_socket, NULL, NULL);
         handshakeMsg handshakeMsg;
         applicationMsg* appMsg = calloc(1, sizeof(applicationMsg));
         readApplicationMsg(tempApplication, appMsg);
         memcpy(&handshakeMsg, appMsg -> payload, sizeof(handshakeMsg));

         //A ping application connected.
          if(handshakeMsg.value == PING)
          {
           if(debug)
           {
             timestamp();
             printf("A PING APPLICATION CONNECTED\n");
           }

            pingApplication = tempApplication;
            tempApplication = -1;
            addEpollEntry(pingApplication, epoll_fd);
           }

          //A routing-deamon connected.
          if(handshakeMsg.value == ROUTING)
          {
           if(debug)
           {
             timestamp();
             printf("A routing_daemon APPLICATION CONNECTED\n");
           }

             routingApplication = tempApplication;
             tempApplication = -1;
             addEpollEntry(routingApplication, epoll_fd);
             sendHandshakeReply(routingApplication, MY_MIP_ADDRESS);
           }

          free(appMsg);
        }

      //closing
      else if(events[i].events & EPOLLHUP)
      {

        //Routing-deamon closed.
        if(events[i].data.fd == routingApplication)
        {
          if(debug)
          {
            timestamp();
            printf("A ROUTING APPLICATION DISCONNECTED\n");
          }

          removeEpollEntry(routingApplication, epoll_fd);
          close(routingApplication);
          routingApplication = -1;
         }

          //Ping applicaiton closed.
        else if(events[i].data.fd == pingApplication)
        {
           removeEpollEntry(pingApplication, epoll_fd);
           close(pingApplication);
           pingApplication = -1;
           if(debug)
           {
             timestamp();
             printf("A PING APPLICATION DISCONNECTED\n");
           }
        }
      }

      //data
      else if (events[i].events & EPOLLIN)
      {
          //Data over ping application.
          if(events[i].data.fd == pingApplication)
          {
            handleApplicationPacket(pingApplication, routingApplication, socket_fd);
          }

          //Data over routing-deamon.
          else if(events[i].data.fd == routingApplication)
          {
            handleRoutingPacket(socket_fd, routingApplication);
          }

          //Data over raw-socket.
          else if(events[i].data.fd == socket_fd)
          {
            handleRawPacket(socket_fd, pingApplication, routingApplication);
          }
      }
    }
  }
}
