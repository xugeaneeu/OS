#include "dir_utils.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

// place for static

static char* get_reversed_path() {
  // implement!!!!!!
}

// place for interface

int create_dir(char *path) {
  int err = mkdir(path, S_IRWXU);
}

char* create_reversed_dir(const char *input_path) {
  unsigned long last_slash = strlen(input_path);
  
  while (*(input_path + last_slash) != '/') {
    --last_slash;
  }

  char *reversed_path = get_reversed_path();

  if (!create_dir(reversed_path)) {
    fprintf(stderr, "Directory wasn't created.\n");
    return NULL;
  } else {
    fprintf(stdout, "Directory %s was created successfully!\n", reversed_path);
    return reversed_path;
  }
}