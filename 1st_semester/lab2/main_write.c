#include <sys/syscall.h>
#include <unistd.h>

void print_hello(void) {
  write(STDOUT_FILENO, "hello\n", 6);
  return;
}

int main(void) {
  print_hello();
  return 0;
}