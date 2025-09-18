#include "dir_utils.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// place for static

static char* get_reversed_path(const char* input_path) {
  unsigned long path_length = strlen(input_path);
  const char* last_slash = strrchr(input_path, '/');

  char* reversed_dir_path = malloc(sizeof(char) * path_length);

  if (NULL == last_slash) {
    for (size_t i = 0; i < path_length; ++i) {
      reversed_dir_path[i] = input_path[path_length - i - 1];
    }
    
    return reversed_dir_path;
  }

  const size_t prefix_length = last_slash - input_path + 1;
  memcpy(reversed_dir_path, input_path, prefix_length);

  size_t suffix_length = path_length - prefix_length;
  for (size_t i = 0; i < suffix_length; ++i) {
    reversed_dir_path[prefix_length + i] = input_path[path_length - i];
  }

  return reversed_dir_path;
}

// place for interface

char* create_reversed_dir(const char *input_path) {
  char *reversed_path = get_reversed_path(input_path);

  if (mkdir(reversed_path, S_IRWXU)) {
    fprintf(stderr, "Directory wasn't created.\n");
    return NULL;
  } else {
    fprintf(stdout, "Directory %s was created successfully!\n", reversed_path);
    return reversed_path;
  }
}