#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
  struct timeval tv, sys_tv;

  //syscall(SYS_gettimeofday, &sys_tv, NULL);
  gettimeofday(&tv, NULL);
  syscall(SYS_gettimeofday, &sys_tv, NULL);
  printf("gettimeofday time: %ld sec %ld usec, tv:%p\n",
    tv.tv_sec, tv.tv_usec, &tv);
  printf("SYS_gettimeofday: %ld sec %ld usec, sys_tv:%p\n",
    sys_tv.tv_sec, sys_tv.tv_usec, &sys_tv);

  pid_t pid = getpid();
  printf("getpid pid = %d\n", pid);

  pid = syscall(SYS_getpid);
  printf("SYS_getpid pid = %d\n", pid);

  return 0;
}
