#include "dir_utils.h"
#include "file_utils.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Incorrect args. Usage: ./reversed_copy [path to file]\n");
    return EXIT_FAILURE;
  }

  char *path = argv[1];
  char *reversed_dir_path = create_reversed_dir(path);
  if (NULL == reversed_dir_path) {
    return EXIT_FAILURE;
  }

  fprintf(stdout, "Reversed directory was created. Start copy files...\n");

  if (make_mirror_files(path, reversed_dir_path)) {
    return EXIT_FAILURE;
  }

  fprintf(stdout, "Program is finished.\n");
  return EXIT_SUCCESS;
}