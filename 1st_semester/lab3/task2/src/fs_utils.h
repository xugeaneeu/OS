#ifndef OS_LAB3_TASK2_FS_UTILS_H
#define OS_LAB3_TASK2_FS_UTILS_H

//with one arg
int create_dir(const char* dir_path);
int show_dir(const char* dir_path);
int rm_dir(const char* dir_path);
int create_file(const char* file_path);
int show_file(const char* file_path);
int rm_file(const char* file_path);
int show_sym_link(const char* path_to_link);
int show_file_by_symlink(const char* path_to_link);
int rm_sym_link(const char* path_to_link);
int rm_hard_link(const char* path_to_link);
int show_file_permissions(const char* path_to_file);

//with two args
int create_sym_link(const char* path_to_file, const char* link_name);
int create_hard_link(const char* path_to_file, const char* link_name);
int change_file_permissions(const char* path_to_file, const char* str_mode);

#endif