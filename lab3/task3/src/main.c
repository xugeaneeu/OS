#include "pagemap_reader.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//takes two args - self or PID of process whose pagemap needs to be read and virtual address, that needs to read 
int main(int argc, char** argv) {
  if (argc != 3) {
    fprintf(stdout, "Usage: ./pid_pagemap_reader [PID] [VIRTUAL ADDRESS]\n");
    return EXIT_FAILURE;
  }

  char path[256] = {};
  if (!strncmp(argv[1], "self", sizeof("self"))) {
    strcpy(path, "/proc/self/pagemap");
  } else {
    char* end;
    int pid = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || pid <= 0) {
      fprintf(stdout, "Usage: ./pid_pagemap_reader [PID] [VIRTUAL ADDRESS]\n");
      return EXIT_FAILURE;
    }
    sprintf(path, "/proc/%d/pagemap", pid);
  }

  char* end;
  const unsigned long vaddr = strtol(argv[2], &end, 16);
  if (end == argv[1] || *end != '\0' || vaddr <= 0) {
    fprintf(stdout, "Usage: ./pid_pagemap_reader [PID] [VIRTUAL ADDRESS]\n");
    return EXIT_FAILURE;
  }

  read_pagemap(path, vaddr);

  return EXIT_SUCCESS;
}