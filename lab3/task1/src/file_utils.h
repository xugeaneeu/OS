#ifndef LAB3_TASK_1_FILE_UTILS_H
#define LAB3_TASK_1_FILE_UTILS_H

#include <stdio.h>

long get_file_size(FILE* f);
void reverse_copy_file_content(FILE* src, FILE* dest);
int make_mirror_files(const char* src, const char* dest);

#endif