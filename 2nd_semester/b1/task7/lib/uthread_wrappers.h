#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


ssize_t uthread_read(int fd, void *buf, size_t count);
ssize_t uthread_write(int fd, const void *buf, size_t count);
ssize_t uthread_send(int sockfd, const void *buf, size_t len, int flags);
ssize_t uthread_recv(int sockfd, void *buf, size_t len, int flags);