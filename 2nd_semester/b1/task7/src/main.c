#include "../lib/uthreads.h"
#include "../lib/uthread_wrappers.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


static void *simple_thread(void *arg) {
  int id = (int)(long)arg;
  for (int i = 0; i < 5; i++) {
    printf("[CPU %d] i = %d\n", id, i);
    uthread_yield();
  }
  return (void *)(long)(id * 100);
}

typedef struct {
  int fd;
  const char *name;
} io_arg_t;

static void *io_writer(void *arg) {
  io_arg_t *a = (io_arg_t *)arg;
  char buf[64];
  ssize_t total = 0;

  for (int i = 0; i < 5; i++) {
    int len = snprintf(buf, sizeof(buf), "[%s] message %d\n", a->name, i);
    ssize_t w = uthread_write(a->fd, buf, len);
    if (w < 0) {
      perror("uthread_write");
      break;
    }
    total += w;
    printf("[%s] wrote %zd bytes\n", a->name, w);
    uthread_yield();
  }
  
  close(a->fd);
  return (void *)(long)total;
}

static void *io_reader(void *arg) {
  io_arg_t *a = (io_arg_t *)arg;
  char buf[128];
  ssize_t tot = 0;
  while (1) {
    ssize_t r = uthread_read(a->fd, buf, sizeof(buf) - 1);
    if (r < 0) {
      perror("uthread_read");
      break;
    }
    if (r == 0) {
      break;
    }
    buf[r] = '\0';
    tot += r;
    printf("[%s] got %zd bytes: %s", a->name, r, buf);
    uthread_yield();
  }
  close(a->fd);
  return (void *)(long)tot;
}

int main(void) {
  if (uthreads_init() != EXIT_SUCCESS) {
    fprintf(stderr, "uthreads_init failed\n");
    return 1;
  }
  
  uthread_t cpu1, cpu2;
  uthread_create(&cpu1, simple_thread, (void *)(long)1);
  uthread_create(&cpu2, simple_thread, (void *)(long)2);
  
  int fds[2];
  if (pipe(fds) < 0) {
    perror("pipe");
    return 1;
  }

  io_arg_t writer_arg = { .fd = fds[1],
                          .name = "writer" };
  io_arg_t reader_arg = { .fd = fds[0],
                          .name = "reader" };
  uthread_t ioW, ioR;
  uthread_create(&ioW, io_writer, &writer_arg);
  uthread_create(&ioR, io_reader, &reader_arg);
  
  uthread_run();
  
  long cpu1_res = (long)uthread_join(&cpu1);
  long cpu2_res = (long)uthread_join(&cpu2);
  long w_res    = (long)uthread_join(&ioW);
  long r_res    = (long)uthread_join(&ioR);

  printf("=== Results ===\n");
  printf("cpu1 returned %ld\n", cpu1_res);
  printf("cpu2 returned %ld\n", cpu2_res);
  printf("writer wrote total %ld bytes\n", w_res);
  printf("reader read  total %ld bytes\n", r_res);
  
  uthread_system_shutdown();

  return 0;
}