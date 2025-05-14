#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int global_var;

int main(void) {
  int local_var = 1;
  global_var = 1;

  printf("My PID: %d\n", getpid());
  printf("Local var addr: %p and value: %d\n", &local_var, local_var);
  printf("Global var addr: %p and value: %d\n", &global_var, global_var);

  pid_t child_pid = fork();
  if (child_pid < 0) {
    printf("Error while forking process\n");
    return EXIT_FAILURE;
  }

  if (child_pid == 0) {
    printf("child proc\n");
    printf("My (child) pid: %d\n", getpid());
    printf("Parrent pid: %d\n", getppid());
    printf("Local  var addr: %p and value: %d\n", &local_var, local_var);
    printf("Global var addr: %p and value: %d\n", &global_var, global_var);
    local_var++;
    global_var++;
    printf("Modified  Local var addr: %p and value: %d\n", &local_var, local_var);
    printf("Modified Global var addr: %p and value: %d\n", &global_var, global_var);
    sleep(20);
    exit(5);
  } else {
    sleep(1);
    printf("parent proc\n");
    printf("Local  var addr: %p and value: %d\n", &local_var, local_var);
    printf("Global var addr: %p and value: %d\n", &global_var, global_var);

    sleep(30);
    int status;
    int terminated_pid = wait(&status);
    if (terminated_pid == -1) {
      printf("Wait error\n");
      return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
      printf("Child process exited with status code: %d\n",
             WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      printf("Child process terminated with unhandled signal with sig number: %d\n",
             WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
      printf("Child process been stoped by a signal with sig number: %d\n",
             WSTOPSIG(status));
    }
  }

  return 0;
}