#ifndef EPOLLFUNCTIONS
#define EPOLLFUNCTIONS

int addEpollEntry(int socket_fd, int epoll_fd);
int removeEpollEntry(int socket_fd, int epoll_fd);

#endif
