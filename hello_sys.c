#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    char *msg = "Hello, world!\n";
    syscall(SYS_write, 1, msg, strlen(msg));

    printf("SYS_write:%d\n", SYS_write);

    return 0;
}
