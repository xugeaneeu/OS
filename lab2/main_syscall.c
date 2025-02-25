#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

ssize_t my_write(int fd, const void *buf, size_t n) {
  return syscall(__NR_write, fd, buf, n);
}

int main(void) {
  ssize_t res = my_write(1, "hello\n", 6);

  if (res == -1) {
    printf("%s \n", strerror(errno));
  }

  return 0;
}