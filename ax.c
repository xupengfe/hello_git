#include <stdio.h>


void main(void)
{
	unsigned long *addr_rax;

	printf("Before give rax to addr_rax:%p, &addr_rax:%p\n\n",
		addr_rax, &addr_rax);

	//movabsq $0x0011223344556677, %rax
	asm volatile("movabsq $0x0011223344556677, %rax");
	asm("movq %%rax,%0" : "=r"(addr_rax));
	printf("After movabsq $0x0011223344556677, rax; addr_rax:%p, &addr_rax:%p\n\n",
		addr_rax, &addr_rax);

	asm volatile("movabsq $0x0011223344556677, %rax");
	asm volatile("movw $-1, %ax");
	asm("movq %%rax,%0" : "=r"(addr_rax));
	printf("After movw $-1, ax; addr_rax:%p, &addr_rax:%p\n\n",
		addr_rax, &addr_rax);

	asm volatile("movabsq $0x0011223344556677, %rax");
	asm volatile("movl $-1, %eax");
	asm("movq %%rax,%0" : "=r"(addr_rax));
	printf("After movq $-1, eax;  addr_rax:%p, &addr_rax:%p\n\n",
		addr_rax, &addr_rax);

	asm volatile("movabsq $0x0011223344556677, %rax");
	asm volatile("movq $-1, %rax");
	asm("movq %%rax,%0" : "=r"(addr_rax));
	printf("After movq $-1, rax; addr_rax:%p, &addr_rax:%p\n",
		addr_rax, &addr_rax);
}
