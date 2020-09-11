#include "epollFunctions.h"

int addEpollEntry(int socket_fd, int epoll_fd) {
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd = socket_fd;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
        perror("epoll_ctl");
        return -1;
    }
    return 0;
}

int removeEpollEntry(int socket_fd, int epoll_fd) {
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd = socket_fd;
    event.events = EPOLLIN | EPOLLHUP; // Not sure if EPOLLHUP is needed.
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket_fd, &event) == -1) {
        perror("epoll_ctl");
        return -1;
    }
    return 0;
}
