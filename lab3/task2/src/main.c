#include "help.h"
#include "utils.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int process_args(int argc, char** argv) {
  if (!strcmp(argv[1], "help")) {
    if (print_help_info() == EXIT_FAILURE) {
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }

  char util_name[FUNC_NAME_SIZE];
  get_util_name(util_name, argv[0]);
  printf("%s\n", util_name);

  printf("Check help command\n");
  return EXIT_FAILURE;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("./fs_utils/[util_name] [PATH_TO_FILE_OR_DIR] [ACTION]\n");
    return EXIT_FAILURE;
  }

  if (process_args(argc, argv)) {
    return EXIT_FAILURE;
  }

  printf("Command has been executed\n");
  return EXIT_SUCCESS;
}