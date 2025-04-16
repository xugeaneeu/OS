#include "help.h"
#include "utils.h"
#include "fs_utils.h"

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

  if (!strcmp(util_name, "create_dir")) {
    return create_dir(argv[1]);
  }
  if (!strcmp(util_name, "show_dir")) {
    return show_dir(argv[1]);
  }
  if (!strcmp(util_name, "rm_dir")) {
    return rm_dir(argv[1]);
  }
  if (!strcmp(util_name, "create_file")) {
    return create_file(argv[1]);
  }
  if (!strcmp(util_name, "show_file")) {
    return show_file(argv[1]);
  }
  if (!strcmp(util_name, "rm_file")) {
    return rm_file(argv[1]);
  }
  if (!strcmp(util_name, "show_sym_link")) {
    return show_sym_link(argv[1]);
  }
  if (!strcmp(util_name, "show_file_by_symlink")) {
    return show_file_by_symlink(argv[1]);
  }
  if (!strcmp(util_name, "rm_sym_link")) {
    return rm_sym_link(argv[1]);
  }
  if (!strcmp(util_name, "rm_hard_link")) {
    return rm_hard_link(argv[1]);
  }
  if (!strcmp(util_name, "show_file_permissions")) {
    return show_file_permissions(argv[1]);
  }
  
  if (argc < 3) {
    printf("Wrong args, chech help");
    return EXIT_FAILURE;
  }
  
  printf("%s %s %s\n", util_name, argv[1], argv[2]);

  if (!strcmp(util_name, "create_sym_link")) {
    return create_sym_link(argv[1], argv[2]);
  }
  if (!strcmp(util_name, "create_hard_link")) {
    return create_hard_link(argv[1], argv[2]);
  }
  if (!strcmp(util_name, "change_file_permissons")) {
    return change_file_permissions(argv[1], argv[2]);
  }

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