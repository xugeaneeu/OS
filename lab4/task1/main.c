#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int global_unitialized;
int global_initialized = 5;
const int global_constant = 10;

void cool_name_function(void) {
  char* a = malloc(100);
  if (NULL == a) {
    printf("memory err\n");
    return;
  }
  strcpy(a, "hello wrld");
  printf("%p %s\n", a, a);
  free(a);
  printf("%p %s\n", a, a);

  char* b = malloc(100);
  if (NULL == b) {
    printf("memory err\n");
    return;
  }
  strcpy(b, "hello wrld");
  printf("%p %s\n", b, b);
}

void some_function(void) {
  int local_unitialized;
  int local_initialized = 20;
  const int local_constant = 40;

  printf("Local_unitialized: %p\nLocal_initialized: %p\nLocal_constant: %p\n",
        &local_unitialized, &local_initialized, &local_constant);
}

int main(void) {
  printf("PID: %d\n", getpid());

  printf("Adresses:\n");
  printf("Global_unitialized: %p\nGlobal_initialized: %p\nGlobal_constant: %p\n",
    &global_unitialized, &global_initialized, &global_constant);

  some_function();

  cool_name_function();

  //TODO: env and do sth with task1

  sleep(120);
  return EXIT_SUCCESS;
}