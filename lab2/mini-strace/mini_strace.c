#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "mini_strace_syscalls.h"

void print_syscall_enter(size_t syscall_num) {
    if (syscall_num < sizeof(syscall_to_name) / sizeof(syscall_to_name[0]))
        fprintf(stderr, "%s", syscall_to_name[syscall_num]);
    else
        fprintf(stderr, "unknown");
}

void print_syscall_exit(size_t return_value) {
    fprintf(stderr, " -> %zu\n", return_value);
}


int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: mini_strace [arg1 [arg12] ...]\n");
    return EXIT_FAILURE;
  }

  pid_t child_pid = fork();
  if (child_pid == -1) {
    fprintf(stdout, "%s\n", strerror(errno));
    return EXIT_FAILURE;
  } else if (child_pid == 0) {
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);

    execvp(argv[1], argv+1);
    fprintf(stdout, "%s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  if (waitpid(child_pid, NULL, 0) == -1) {
    fprintf(stdout, "%s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  while(1) {
    struct user_regs_struct regs;

    if (ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL) == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      return EXIT_FAILURE;
    }
    if (waitpid(child_pid, NULL, 0) == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      return EXIT_FAILURE;
    }

    if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      return EXIT_FAILURE;
    }

    //fprintf(stdout, "%llu\n", regs.orig_rax); //change to print syscall
    print_syscall_enter(regs.orig_rax);

    if (ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL) == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      return EXIT_FAILURE;
    }
    if (waitpid(child_pid, NULL, 0) == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      return EXIT_FAILURE;
    }

    if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) == -1) {
      if (errno == ESRCH) {
        fprintf(stderr, "\nTracing is finished\n");
        break;
      }

      fprintf(stderr, "%s\n", strerror(errno));
      return EXIT_FAILURE;
    }

    //fprintf(stdout, "%llu\n", regs.orig_rax); //change to print syscall
    print_syscall_exit(regs.rax);
  }

  return EXIT_SUCCESS;
}