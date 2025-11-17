#include "uthread_wrappers.h"
#include "uthreads.h"

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

/*----------------Static functions-----------------*/

static int ensure_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) return -1;
  
  if (!(flags & O_NONBLOCK) 
      && fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return -1;
  
  return 0;
}

/*----------------API-----------------*/

ssize_t uthread_read(int fd, void *buf, size_t count) {
  if (ensure_nonblocking(fd) < 0) {
    return -1;
  }
  ssize_t ret;
  while (1) {
    ret = read(fd, buf, count);
    if (ret >= 0) return ret;

    if (errno == EINTR) continue;
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      uthread_yield();
      continue;
    }
    return -1;
  }
}

ssize_t uthread_write(int fd, const void *buf, size_t count) {
  if (ensure_nonblocking(fd) < 0) return -1;

  ssize_t ret;
  while (1) {
    ret = write(fd, buf, count);
    if (ret >= 0) return ret;
    if (errno == EINTR) continue;
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      uthread_yield();
      continue;
    }
    return -1;
  }
}

ssize_t uthread_send(int sockfd, const void *buf, size_t len, int flags) {
  if (ensure_nonblocking(sockfd) < 0) return -1;

  ssize_t ret;
  while (1) {
    ret = send(sockfd, buf, len, flags);
    if (ret >= 0) return ret;
    if (errno == EINTR) continue;
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      uthread_yield();
      continue;
    }
    return -1;
  }
}

ssize_t uthread_recv(int sockfd, void *buf, size_t len, int flags) {
  if (ensure_nonblocking(sockfd) < 0) return -1;
  
  ssize_t ret;
  while (1) {
    ret = recv(sockfd, buf, len, flags);
    if (ret >= 0) return ret;
    if (errno == EINTR) continue;
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      uthread_yield();
      continue;
    }
    return -1;
  }
}