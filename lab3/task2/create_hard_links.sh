#!/bin/bash

rm -rf fs_utils && mkdir fs_utils

for link in "create_dir" "show_dir" "rm_dir" "create_file" "show_file" "rm_file" "create_sym_link" "show_sym_link" "show_file_by_symlink" "rm_sym_link" "create_hard_link" "rm_hard_link" "show_file_permissions" "change_file_permissons"
do
  ln ./fs_utility /fs_utils/$link
done