#include "rawFunctions.h"
#include "protocol.h"

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
}

int sendRawPacket(int socket, struct sockaddr_ll *socketname, mip_header mip_header, char* buffer, int len, uint8_t dst_addr[])
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

  msgvec[1].iov_base = &mip_header;
  msgvec[1].iov_len = sizeof(struct mip_header);

  //padding to make SDU 32-bit alligned.
  int counter = len;
  while((sizeof(struct ethernet_header) + sizeof(mip_header)  + counter)%4 != 0)
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
    free(msg);
    return 1;
  }
  printf("\n Sendt %d bytes \n", bytes);
}
