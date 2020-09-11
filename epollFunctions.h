#ifndef EPOLLFUNCTIONS
#define EPOLLFUNCTIONS
#include "protocol.h"

int addEpollEntry(int socket_fd, int epoll_fd);
int removeEpollEntry(int socket_fd, int epoll_fd);

#endif
