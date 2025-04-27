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
  printf("%s\n", a);
  free(a);
  printf("%s\n", a);

  char* b = malloc(100);
  if (NULL == b) {
    printf("memory err\n");
    return;
  }
  strcpy(b, "hello wrld");
  printf("%s\n", b);
  free(b + 50);
  printf("%s\n", b);
  free(b);
}

int* get_local(void) {
  int local = 10;
  return &local;
}

void print_addr(void) {
  int local_unitialized;
  int local_initialized = 20;
  const int local_constant = 40;

  printf("Adresses:\n");
  printf("Global_unitialized: %p\nGlobal_initialized: %p\nGlobal_constant: %p\n",
        &global_unitialized, &global_initialized, &global_constant);
  printf("Local_unitialized: %p\nLocal_initialized: %p\nLocal_constant: %p\n",
        &local_unitialized, &local_initialized, &local_constant);
}

int main(void) {
  printf("PID: %d\n", getpid());

  #ifdef a
  print_addr();
  sleep(720);
  #endif

  #ifdef d
  printf("%p", get_local());
  #endif

  #ifdef e
  cool_name_function();
  #endif

  #ifdef g
  char* env_val = getenv("MY_ENV_VAR");
  if(env_val != NULL) {
    printf("Init value of MY_ENV_VAR: %s\n", env_val);
  }
  else {
    printf("MY_ENV_VAR is not set\n");
  }
  #endif

  return EXIT_SUCCESS;
}