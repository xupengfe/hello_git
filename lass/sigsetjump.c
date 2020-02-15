#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>

jmp_buf jmpbuf;

void printBlock() {
  sigset_t block;
  sigprocmask(SIG_BLOCK, NULL, &block);
  printf("block:");
  if (sigismember(&block, SIGQUIT)) printf("SIGQUIT, ");
  if (sigismember(&block, SIGALRM)) printf("SIGALRM");
  puts("");
}

void handler(int sig) {
  if (sig == SIGQUIT) printf("SIGQUIT, ");
  if (sig == SIGALRM) printf("SIGALRM, ");
  printBlock();
  //printf("jmpbuf: %lx\n", jmpbuf);
  siglongjmp(jmpbuf, 1);
  puts("--------------------------------------------------");
}

int main() {
  printf("I'm %d\n", getpid());
  signal(SIGQUIT, handler);
  signal(SIGALRM, handler);
  printf("before signal, ");
  printBlock();

  if (sigsetjmp(jmpbuf, 1) != 0) {
    printf("jump to here! ");
    printBlock();
    puts("====================================================");
  }

  while(1) {
    pause();
  }
  return 0;
}
