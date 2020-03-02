#include <sys/utsname.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
    struct utsname n;
    uname(&n);
    printf(" sysname:%s\n nodename:%s\n release:%s\n version:%s\n machine:%s\n ",\
                    n.sysname,\
                    n.nodename,\
                    n.release,\
                    n.version,\
                    n.machine);
    return 0;
}
