#include <stdio.h>
#include <stdlib.h>
#include <x86intrin.h>


static int clflush(void)
{
//	unsigned long *m8;
	int *m8;
	int content[8];
	m8 = content;
	content[0] = 6;

	printf("m8 addr:%p, *m8:0x%x, *(m8+1):0x%x\n",
		m8, *m8, *(m8+1));
	_mm_clflush(m8);
	//asm volatile("clflush m8");
	printf("m8 addr:%p, *m8:0x%x, *(m8+1):0x%x\n",
		m8, *m8, *(m8+1));

	return 0;
}

int main(void)
{
	clflush();

	return 0;
}
