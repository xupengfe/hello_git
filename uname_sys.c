#include <sys/utsname.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	struct utsname n;
	void *s;

	s = &n;

	asm volatile(
			"movl %0,%%ebx\n\t"
			"movl $0x7A,%%eax\n\t"
			"int $0x80\n\t"
			:"=m"(s)
			);

	printf(" sysname:%s\n nodename:%s\n release:%s\n version:%s\n machine:%s\n ",\
			n.sysname, n.nodename, n.release, n.version, n.machine);
	return 0;
}
