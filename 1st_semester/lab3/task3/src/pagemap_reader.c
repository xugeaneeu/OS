#include "pagemap_reader.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define PAGEMAP_ENTRY 8

#define GET_BIT(val, bit) (((val) >> (bit)) & 1)


void read_pagemap(const char *path, unsigned long vaddr) {
  FILE* f = fopen(path, "rb"); //b for windows
  if (!f) {
    fprintf(stdout, "Can't open file by path %s\n", path);
    exit(EXIT_FAILURE);
  }

  //seeking to vaddr offset
  int page_size = getpagesize();
  uint64_t file_offset = (vaddr / page_size) * PAGEMAP_ENTRY;

  if (fseek(f, file_offset, SEEK_SET) != 0) {
    fprintf(stdout, "Failed to fseek\n");
    fclose(f);
    exit(EXIT_FAILURE);
  }

  uint64_t vpage_value;
  if (fread(&vpage_value, PAGEMAP_ENTRY, 1, f) != 1) {
    fprintf(stdout, "Failed to read pagemap of virtual address: %lu\n", vaddr);
    fclose(f);
    exit(EXIT_FAILURE);
  }

  fclose(f);

  printf("Result: 0x%llx\n", (unsigned long long)vpage_value);

  if (GET_BIT(vpage_value, 63)) {
    printf("Page present\n");
  } else if (GET_BIT(vpage_value, 62)) {
    printf("Page swapped\n");
  } else {
    printf("Page not present\n");
  }
}