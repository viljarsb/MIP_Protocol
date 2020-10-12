#include "epollFunctions.h"
#include <sys/epoll.h> //Epoll
#include <string.h> //memset.
#include <stdlib.h>
#include <stdio.h>
#include "log.h"

/*
    This file contains utility functions to add and remove file-descriptors
    from a epoll-set. Provides a cleaner looking use of epoll.
*/



/*
    This function adds another socket-fd to a epollset, so it can be monitored.

    @Param  the socket fd to add and the epoll-fd to add it to.
    @Return  a int, 0 for ok and termination of program for fail.
*/
int addEpollEntry(int socket_fd, int epoll_fd) {
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd = socket_fd;
    event.events = EPOLLIN | EPOLLHUP; //Monitor for data and peer-shutdown.
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
        timestamp();
        printf("epoll_ctl_add\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

/*
    This function removes a socket-fd from a epollset, so its no longer monitored.
    @Param  the socket-fd to remove and the epoll-fd to remove it from.
    @Return  a int, 0 for ok and termination of program for fail.
*/
int removeEpollEntry(int socket_fd, int epoll_fd) {
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd = socket_fd;
    event.events = EPOLLIN | EPOLLHUP;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket_fd, &event) == -1) {
        timestamp();
        printf("epoll_ctl_del\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}
