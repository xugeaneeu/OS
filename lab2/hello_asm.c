#include <sys/syscall.h>
#include <unistd.h>

long my_syscall(long sysno, long arg1, long arg2, long arg3,
  long arg4, long arg5, long arg6) {
  long ret;
  asm volatile (
  "mov    %%rdi, %%rax\n\t" // rax = номер системного вызова (из первого параметра rdi).
  "mov    %%rsi, %%rdi\n\t" // rdi = arg1 (был в rsi).
  "mov    %%rdx, %%rsi\n\t" // rsi = arg2 (был в rdx).
  "mov    %%rcx, %%rdx\n\t" // rdx = arg3 (был в rcx).
  "mov    %%r8,  %%r10\n\t" // r10 = arg4 (был в r8).
  "mov    %%r9,  %%r8\n\t" // r8 = arg5 (был в r9).
  "mov    8(%%rsp), %%r9\n\t" // r9 = аргумент, переданный по стеку (arg6).
  "syscall\n\t"               // Выполнение системного вызова.
  : "=a" (ret)  // возвращаемое значение получится в rax
  : "D" (sysno),   // исходно: rdi = номер системного вызова
  "S" (arg1),  // rsi = 1-й аргумент
  "d" (arg2),  // rdx = 2-й аргумент
  "c" (arg3),  // rcx = 3-й аргумент
  "r" (arg4),  // arg5 и arg6 передаются через r8, r9
  "r" (arg5),
  "r" (arg6)
  );

  return ret;
}

int main(void) {
  const char msg[] = "hello\n";
  long len = sizeof(msg) - 1;

  my_syscall(SYS_write, STDOUT_FILENO, (long)msg, len, 0, 0, 0);

  // pause();

  // my_syscall(SYS_write, STDOUT_FILENO, (long)msg, len, 0, 0, 0);

  return 0;
}