#include "file_utils.h"

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

//return dot index or -1 if no dot
static int get_dot_pos(const char* file_name) {
  const char* dot = strrchr(file_name, '.');
  return dot ? (dot - file_name) : -1;
}

static char* reverse_filename(const char* filename){
  size_t file_length = strlen(filename);
  char *rev_name = malloc(file_length);
  if (NULL == rev_name) {
    fprintf(stderr, "Memory error.\n");
    return NULL;
  }
  int dot_index = get_dot_pos(filename);
  size_t base_length = (dot_index == -1) ? file_length : dot_index;

  for (size_t i = 0; i < base_length; i++) {
      rev_name[i] = filename[base_length - 1 - i];
  }

  if (dot_index != -1) {
      strcpy(rev_name + base_length, filename + base_length);
  }

  return rev_name;
}

static char* get_full_path (const char* path, const char* filename) {
  char* full_path = calloc(strlen(path) + strlen(filename), sizeof(char));
  if (NULL == full_path) {
    fprintf(stderr, "Memory error.\n");
    return NULL;
  }
  strcpy(full_path, path);
  strcat(full_path, "/");
  strcat(full_path, filename);

  return full_path;
}

static int create_mirror_file(char* src_path, char* dest_path) {
  FILE* fin = fopen(src_path, "r");
  if (NULL == fin) {
    fprintf(stderr, "Can't open source file %s\n", src_path);
    return EXIT_FAILURE;
  }
  FILE* fout = fopen(dest_path, "w");
  if (NULL == fout) {
    fprintf(stderr, "Can't create dest file %s\n", dest_path);
    return EXIT_FAILURE;
  }
  reverse_copy_file_content(fin, fout);
  if (fclose(fout) != 0) {
    fprintf(stderr, "Can't close src file %s\n", src_path);
    return EXIT_FAILURE;
  }
  if (fclose(fin) != 0) {
    fprintf(stderr, "Can't close dest file %s\n", dest_path);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

long get_file_size(FILE *f) {
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  rewind(f);
  return size;
}

void reverse_copy_file_content(FILE *src, FILE *dest) {
  long src_size = get_file_size(src);
  int fseek_flag = fseek(src, -1, SEEK_END);

  long i = src_size;
  while (i > 0 && fseek_flag == 0) {
    char current_char = fgetc(src);
    fprintf(dest, "%c", current_char);
    fseek_flag = fseek(src, -2, SEEK_CUR);
    i--;
  }
}

int make_mirror_files(const char* src, const char* dest) {
  DIR* dir = opendir(src);
  if (NULL == dir) {
    fprintf(stderr, "Can't open the selected directory for copy.\n");
    return EXIT_FAILURE;
  }
  
  char* file_src = calloc(strlen(src) + 1, sizeof(char));
  char* file_dest = calloc(strlen(dest) + 1, sizeof(char));
  strcpy(file_src, src);
  strcpy(file_dest, dest);
  strcat(file_src, "/\0");
  strcat(file_dest, "/\0");

  struct stat dirent_stat;
  struct dirent* dirent_ptr;
  while (NULL != (dirent_ptr = readdir(dir))) {
    char* src_file_path = get_full_path(src, dirent_ptr->d_name);
    if (NULL == src_file_path) {
      return EXIT_FAILURE;
    }
    stat(src_file_path, &dirent_stat);
    if (S_ISREG(dirent_stat.st_mode) != 0) {
      char* reversed_name = reverse_filename(dirent_ptr->d_name);
      if (NULL == reversed_name) {
        return EXIT_FAILURE;
      }
      char* dest_file_path = get_full_path(dest, reversed_name);
      if (NULL == dest_file_path) {
        return EXIT_FAILURE;
      }
      if (create_mirror_file(src_file_path, dest_file_path)) {
        fprintf(stderr, "Reversed file %s didn't created", reversed_name);
        free(file_src);
        free(file_dest);
        free(src_file_path);
        free(dest_file_path);
        return EXIT_FAILURE;
      }
      free(reversed_name);
      free(dest_file_path);
    }
    free(src_file_path);
  }

  if (closedir(dir) == -1) {
    fprintf(stderr, "Can't close correctly the selected directory.\n");
    return EXIT_FAILURE;
  }
  free(file_src);
  free(file_dest);

  return EXIT_SUCCESS;
}