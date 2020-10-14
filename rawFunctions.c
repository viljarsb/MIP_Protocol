#include <string.h> //memcpy.
#include <stdio.h> //printf.
#include <arpa/inet.h> //msghdr, recvmsg etc..
#include "rawFunctions.h" //Singatures of this file.
#include "interfaceFunctions.h" //Interfaces.
#include "arpFunctions.h" //Arp funtionality.
#include "msgQ.h" //Q for msgs.
#include "log.h" //timestamp.

extern int debug; //Extern debugflag from mip-deamon.
extern list* interfaces; //Extern linkedList of interfaces from the mip-deamon.
extern msgQ* waitingQ;  //Extern q of waiting Msgs from the mip-deamon.
extern list* arpWaitingList; //Extern linkedList of Msgs waiting for arp-responses.


/*
    This file contains funtions to read and send data over a socket
    to a remote host.
*/



/*
    This function read from a raw-socket into specified memory locations.

    @Param  A raw socket, pointers to a ethernet_header, a mip_header, a buffer and a int (interface).
    @Return  A int (amount of bytes read).
*/
int readRawPacket(int socket_fd, ethernet_header* ethernet_header, mip_header* mip_header, char* payload, int* interface)
{
  //Variables and structues needed to read from socket.
  int bytes;
  struct sockaddr_ll socket_addr;
	struct msghdr msg;
  memset(&msg, 0, sizeof(struct msghdr));
  //3 scattered buffers.
	struct iovec msgvec[3];

  //Read in a ethernet_header, a mip_header and a data buffer.
  msgvec[0].iov_base = ethernet_header;
  msgvec[0].iov_len  = sizeof(struct ethernet_header);
  msgvec[1].iov_base = mip_header;
  msgvec[1].iov_len  = sizeof(struct mip_header);
  msgvec[2].iov_base = payload;
  msgvec[2].iov_len = 1024;

  msg.msg_name    = &socket_addr;
  msg.msg_namelen = sizeof(struct sockaddr_ll);
  msg.msg_iovlen  = 3;
  msg.msg_iov     = msgvec;

	bytes = recvmsg(socket_fd, &msg, 0);
  //Copy the interface the packet was recived on into the pointer interface.
  memcpy(interface, &socket_addr.sll_ifindex, sizeof(int));

  if(debug)
  {
    timestamp();
    printf("RECEIVED %d BYTES ON INTERFACE: %d\n", bytes, socket_addr.sll_ifindex);
    char* src = getMacFormat(ethernet_header -> src_addr);
    timestamp();
    printf("RECEIVED ETHERNETFRAME -- SRC ETHERNET: %s -- ", src);
    char* dst = getMacFormat(ethernet_header -> dst_addr);
    printf("DST ETHERNET: %s\n\n", dst);
  }
  return bytes;
}



/*
    This function writes to a raw socket from specified memory locations.

    @Param  A raw socket, pointers to a sockaddr_ll, a mip_header, a buffer, the length of the buffer and the destination mac.
    @Return  A int (amount of bytes sent).
*/
int sendRawPacket(int socket, struct sockaddr_ll *socketname, mip_header* mip_header, char* buffer, int len, uint8_t dst_addr[])
{
  int bytes;
	struct ethernet_header ethernet_frame;
	struct msghdr *msg;
	struct iovec msgvec[3];

  //Fill in the ethernet header.
  memcpy(ethernet_frame.dst_addr, dst_addr, 6);
  memcpy(ethernet_frame.src_addr, socketname -> sll_addr, 6);
  ethernet_frame.protocol = htons(ETH_P_MIP);

  msgvec[0].iov_base = &ethernet_frame;
  msgvec[0].iov_len  = sizeof(struct ethernet_header);
  msgvec[1].iov_base = mip_header;
  msgvec[1].iov_len = sizeof(struct mip_header);

  //padding to make SDU 32-bit alligned.
  int counter = len;
  while(counter%4 != 0)
  {
    counter = counter + 1;
  }

  char* bufferPadded;
  bufferPadded = calloc(counter,  sizeof(char));
  memcpy(&bufferPadded[0], buffer, len);

  msgvec[2].iov_base = bufferPadded;
  msgvec[2].iov_len = counter;

  msg = (struct msghdr *)calloc(1, sizeof(struct msghdr));
  //Fill in metadata.
  memcpy(socketname->sll_addr, &dst_addr, 6);
  msg->msg_name    = socketname;
  msg->msg_namelen = sizeof(struct sockaddr_ll);
  msg->msg_iovlen  = 3;
  msg->msg_iov     = msgvec;

  /* Construct and send message */
  bytes = sendmsg(socket, msg, 0);
  if(bytes == -1) {
    printf("sendmsg\n");
    return -1;
  }

  if(debug)
  {
    timestamp();
    char* src = getMacFormat(ethernet_frame.src_addr);
    printf("SENDING ETHERNETFRAME -- SRC ETHERNET %s -- ", src);
    char* dst = getMacFormat(ethernet_frame.dst_addr);
    printf("DST ETHERNET: %s\n", dst);
    timestamp();
    printf("SENDT %d BYTES OF DATA OVER INTERFACE %d\n\n", bytes, socketname -> sll_ifindex);
  }
  free(bufferPadded);
  free(msg);
  return bytes;
}



/*
    This is the function the program should interact with when sending data to
    remote nodes, sort of like a api.
    The programs just supply the mip_header, the data and the destination mip.
    The rest is figured out dynamicly.

    This function looks at the dst_mip supplied to determine where to send the packet.
    If its broadcast, just broadcast over every interface this node has.
    If its not a broadcast, find the correct arp-entry, and get the next jump.

    @Param  A raw-socket, pointer to mip-header, data and a destination MIP-address.
*/
void sendData(int socket_fd, mip_header* mip_header, char* buffer, u_int8_t mipDst)
{
  //Send over broadcast mac if destination mip is 255.
  if(mip_header -> dst_addr == 0xFF && mipDst == 0xFF)
  {
    uint8_t dst_addr[ETH_ALEN] = BROADCAST_MAC_ADDR;
    node* tempInterface = interfaces -> head;
    struct sockaddr_ll temp;

    while(tempInterface != NULL)
    {
      interface* currentInterface = (struct interface*) tempInterface -> data;
      memcpy(&temp, &currentInterface -> sock_addr, sizeof(struct sockaddr_ll));
      sendRawPacket(socket_fd, &temp, mip_header, buffer, mip_header -> sdu_length, dst_addr);
      tempInterface = tempInterface -> next;
    }
    free(mip_header);
    free(buffer);
    return;
  }

  //Check cache.
  arpEntry* entry = getCacheEntry(mipDst);

  //If not entry is found for that mip, store the msg and send a arp-broadcast.
  if(entry == NULL)
  {
    struct arpWaitEntry arpWaitEntry;
    arpWaitEntry.dst = mipDst;

    arpWaitEntry.mip_header = mip_header;
    arpWaitEntry.buffer = buffer;
    addEntry(arpWaitingList, &arpWaitEntry);
    sendArpBroadcast(socket_fd, interfaces, mipDst);
    return;
  }

  //find the interface to send on.
  interface* interfaceToUse = getInterface(interfaces, entry -> via_interface);
  struct sockaddr_ll temp;
  memcpy(&temp, &interfaceToUse -> sock_addr, sizeof(struct sockaddr_ll));

  //send the pointers over to sendRawPacket, and free the mip_header and buffer.
  sendRawPacket(socket_fd, &temp, mip_header, buffer, mip_header -> sdu_length, entry -> mac_address);
  free(mip_header);
  free(buffer);
}
