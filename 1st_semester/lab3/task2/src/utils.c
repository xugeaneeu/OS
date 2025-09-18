#include "utils.h"
#include <string.h>

//place for static functions

static int get_name_idx(char* path) {
  int length = strlen(path);
  int start_name_idx = 0;
  for (int i = length-1; i > 0; --i) {
    if (path[i] == '/') {
      start_name_idx = i + 1;
      break;
    }
  }

  return start_name_idx;
}

//place for interface

void get_util_name(char *buffer, char *file_name) {
  int start_idx = get_name_idx(file_name);
  int file_name_size = strlen(file_name);

  for (int i = start_idx; i < file_name_size; ++i) {
    buffer[i - start_idx] = file_name[i];
  }
}