#include "rawFunctions.h"
#include "interfaceFunctions.h"
#include "arpFunctions.h"
#include "msgQ.h"
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include "log.h"
#include <arpa/inet.h>

extern int debug;
extern list* interfaces;
extern msgQ* waitingQ;
extern msgQ* arpQ;

int readRawPacket(int socket_fd, ethernet_header* ethernet_header, mip_header* mip_header, char* payload, int* interface)
{
  int rc;
  struct sockaddr_ll socket_addr;
	struct msghdr msg;
  memset(&msg, 0, sizeof(struct msghdr));
	struct iovec msgvec[3];

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

	rc = recvmsg(socket_fd, &msg, 0);
  memcpy(interface, &socket_addr.sll_ifindex, sizeof(int));

  if(debug)
  {
    timestamp();
    printf("RECEIVED %d BYTES ON INTERFACE: %d\n", rc, socket_addr.sll_ifindex);
    char* src = getMacFormat(ethernet_header -> src_addr);
    printf("RECEIVED ETHERNETFRAME -- SRC ETHERNET: %s -- ", src);
    char* dst = getMacFormat(ethernet_header -> dst_addr);
    printf("DST ETHERNET: %s\n\n", dst);
  }
  return rc;
}

int sendRawPacket(int socket, struct sockaddr_ll *socketname, mip_header* mip_header, char* buffer, int len, uint8_t dst_addr[])
{
  int bytes;
	struct ethernet_header ethernet_frame;
	struct msghdr *msg;
	struct iovec msgvec[3];

  /* Fill in Ethernet header */
  memcpy(ethernet_frame.dst_addr, dst_addr, 6);
  memcpy(ethernet_frame.src_addr, socketname -> sll_addr, 6);
  /* Match the ethertype in packet_socket.c: */
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

  /* Allocate a zeroed-out message info struct */
  msg = (struct msghdr *)calloc(1, sizeof(struct msghdr));
  /* Fill out message metadata struc10t */
  memcpy(socketname->sll_addr, &dst_addr, 6);
  msg->msg_name    = socketname;
  msg->msg_namelen = sizeof(struct sockaddr_ll);
  msg->msg_iovlen  = 3;
  msg->msg_iov     = msgvec;

  /* Construct and send message */
  bytes = sendmsg(socket, msg, 0);
  if (bytes == -1) {
    perror("sendmsg");
    return 1;
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

void sendApplicationData(int socket_fd, mip_header* mip_header, char* buffer, u_int8_t mipDst)
{
  if(mip_header -> dst_addr == 0xFF)
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

  arpEntry* entry = getCacheEntry(mipDst);
  if(entry == NULL)
  {
    struct arpWaitEntry* arpWaitEntry = malloc(sizeof(struct arpWaitEntry));
    arpWaitEntry -> dst = mipDst;

    //FIND ANOTHER WAY!!
    struct mip_header* header = malloc(sizeof(mip_header));
    memcpy(header, mip_header, sizeof(mip_header));
    char* buffer2 = malloc(mip_header -> sdu_length);
    memcpy(buffer2, buffer, mip_header  -> sdu_length);
    free(mip_header);
    free(buffer);

    arpWaitEntry -> mip_header = header;
    arpWaitEntry -> buffer = buffer2;
    push(arpQ, arpWaitEntry);
    sendArpBroadcast(socket_fd, interfaces, mipDst);
    return;
  }

  interface* interfaceToUse = getInterface(interfaces, entry -> via_interface);
  struct sockaddr_ll temp;
  memcpy(&temp, &interfaceToUse -> sock_addr, sizeof(struct sockaddr_ll));
  sendRawPacket(socket_fd, &temp, mip_header, buffer, mip_header -> sdu_length, entry -> mac_address);
  free(mip_header);
  free(buffer);
}
