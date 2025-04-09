#include "help.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define PAGE_SIZE 4096

//place for interface

int print_help_info(void) {
  FILE* help_info = fopen("../resources/help_info.txt", "rb");
  if (!help_info) {
    printf("Check /resources/help_info.txt\n");
    return EXIT_FAILURE;
  }
  fseek(help_info, 0, SEEK_END);
  long file_size = ftell(help_info);
  rewind(help_info);

  char buffer[PAGE_SIZE];
  int read = 0;
  for (long i = 0; i < file_size; i += 4096) {
    read = fread(buffer, sizeof(char), PAGE_SIZE, help_info);
    write(STDIN_FILENO, buffer, read);
  }

  fclose(help_info);
  return EXIT_SUCCESS;
}