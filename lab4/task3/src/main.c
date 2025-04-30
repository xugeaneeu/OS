#include "my_malloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  int ret = init_heap();
  if(ret == -1) {
      return EXIT_FAILURE;
  }

  char* ptr1 = (char*)my_malloc(strlen("hellllo wrld") * sizeof(char));
  if(ptr1 == NULL) {
      printf("my_malloc error\n");
      return EXIT_FAILURE;
  }
  printf("Allocated ptr1 at address: %p\n", ptr1);

  const char* message = "hellllo wrld";
  memcpy(ptr1, message, strlen(message));
  printf("%s\n", ptr1);

  my_free(ptr1);
  printf("Freed ptr1\n");
  
  destroy_heap();
  return EXIT_SUCCESS;
}