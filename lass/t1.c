#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
  struct timeval tv, sys_tv;
  struct timeval *tp = &tv, *stp = &sys_tv;
  unsigned int hack_a;


  printf("SYS_gettimeofday:%d\n", SYS_gettimeofday);
  gettimeofday(&tv, NULL);
  printf("gettimeofday time: %ld sec %ld usec, &tv:%p\n",
    tv.tv_sec, tv.tv_usec, &tv);
  syscall(SYS_gettimeofday, &sys_tv, NULL);
  printf("gettimeofday time: %ld sec %ld usec, &tv:%p\n",
    tv.tv_sec, tv.tv_usec, &tv);
  printf("SYS_gettimeofday: %ld sec %ld usec, sys_tv:%p, %p, %p, %p\n",
    sys_tv.tv_sec, sys_tv.tv_usec, &sys_tv, stp, &stp, &(*stp));

  pid_t pid = getpid();
  printf("getpid pid = %d\n", pid);

  pid = syscall(SYS_getpid);
  printf("SYS_getpid pid = %d\n", pid);

  //hack_a = *(const int *)0xffffffffff600000;
  //printf("0xffffffffff600000:0x%x\n", hack_a);
  // hack_a = *(const int *)0xffffffffff601000; // will SIGSEGV
  return 0;
}
