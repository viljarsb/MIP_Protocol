#include "epollFunctions.h"
#include <sys/epoll.h>
#include <string.h>
#include <stdio.h>

/*
    This function adds another socket-fd to a epollset, so it can be monitored.
    @Param  the socket fd to add and the epoll-fd to add it to.
    @Return  a int, 0 for ok and -1 for fail.
*/
int addEpollEntry(int socket_fd, int epoll_fd) {
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd = socket_fd;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
        perror("epoll_ctl_add");
        return -1;
    }
    return 0;
}

/*
    This function removes a socket-fd from a epollset, so its no longer monitored.
    @Param  the socket-fd to remove and the epoll-fd to remove it from.
    @Return  a int, 0 for ok and -1 for fail.
*/
int removeEpollEntry(int socket_fd, int epoll_fd) {
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd = socket_fd;
    event.events = EPOLLIN | EPOLLHUP;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket_fd, &event) == -1) {
        perror("epoll_ctl_del");
        return -1;
    }
    return 0;
}
