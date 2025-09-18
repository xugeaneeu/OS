#include "fs_utils.h"

#include <stddef.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

//place for statics

static long get_file_size(FILE* file) {
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);
  return file_size;
}

static mode_t get_mode(const char* str_mode) {
  char* end_ptr = NULL;
  mode_t permissions = strtol(str_mode, &end_ptr, 8);
  if (end_ptr == str_mode || *end_ptr != '\0' || permissions > 0777) {
      printf("Wrong mode\n");
      return EXIT_FAILURE;
  }
  return permissions;
}

//directory utils

int create_dir(const char *dir_path) {
  if (mkdir(dir_path, S_IRWXU)) {
    printf("mkdir err\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int show_dir(const char* dir_path) {
  struct dirent** dirents;
  int num = scandir(dir_path, &dirents, NULL, alphasort);
  if (num == -1) {
    printf("scandir err\n");
    return EXIT_FAILURE;
  }
  for (int i = 0; i < num; ++i) {
    printf("%s\n", dirents[i]->d_name);
    free(dirents[i]);
  }

  free(dirents);
  return EXIT_SUCCESS;
}

int rm_dir(const char* dir_path) {
  if (rmdir(dir_path)) {
    printf("rmdir err\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

//regular file utils

int create_file(const char* file_path) {
  FILE* new = fopen(file_path, "w");
  if (!new) {
    printf("file didn't created\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int show_file(const char* file_path) {
  FILE* f = fopen(file_path, "r");
  if (!f) {
    printf("Can't open the file\n");
    return EXIT_FAILURE;
  }

  long file_size = get_file_size(f);

  char buffer[BUFFER_SIZE];
  long read_bytes = 0;
  for (size_t i = 0; i < file_size; i += 4096) {
    read_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, f);
    fwrite(buffer, sizeof(char), BUFFER_SIZE, stdout);
  }

  fclose(f);
  return EXIT_SUCCESS;
}

int rm_file(const char* file_path) {
  return rm_hard_link(file_path);
}

int create_sym_link(const char* path_to_file, const char* link_name) {
  if (symlink(path_to_file, link_name)) {
    printf("Can't create symbol link\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int show_sym_link(const char* path_to_link) {
  char buffer[BUFFER_SIZE];
  if (!readlink(path_to_link, buffer, BUFFER_SIZE)) {
    printf("Can't read the link\n");
    return EXIT_FAILURE;
  }
  printf("%s\n", buffer);
  return EXIT_SUCCESS;
}

int show_file_by_symlink(const char* path_to_link) {
  char file_path[BUFFER_SIZE];
  if (readlink(path_to_link, file_path, BUFFER_SIZE)) {
    printf("Can't read the link\n");
    return EXIT_FAILURE;
  }
  return show_file(file_path);
}

int rm_sym_link(const char* path_to_link) {
  return rm_hard_link(path_to_link);
}

int create_hard_link(const char* path_to_file, const char* link_name) {
  if (link(path_to_file, link_name)) {
    printf("Can't create hard link to file\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int rm_hard_link(const char* path_to_link) {
  if (unlink(path_to_link)) {
    printf("Can't del hard link\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int show_file_permissions(const char* path_to_file) {
  struct stat file_status;
  if (stat(path_to_file, &file_status)) {
    printf("Can't get file mode\n");
    return EXIT_FAILURE;
  }
  printf("File permissions: %x", file_status.st_mode);
  return EXIT_SUCCESS;
}

int change_file_permissions(const char* path_to_file, const char* str_mode) {
  mode_t mode = get_mode(str_mode);
  if (mode == EXIT_FAILURE || chmod(path_to_file, mode)) {
    printf("Can't change the mode\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}