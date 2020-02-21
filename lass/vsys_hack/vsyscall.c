// compiled: gcc -g vsyscall.c -o vsyscall
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void backdoor();

int main()
{
    char buf[0x100];
    read(0, buf, 0x100 - 1);
    // 直接跳转到buf
    asm("jmp  %0" :  : "m" (buf));

    return 0;
}

void backdoor()
{
    execve("/bin/sh", NULL, NULL);
}