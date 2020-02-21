// compiled: gcc test.c -g -o test
int main()
{
	asm(// str: /bin/sh
//		"mov $0x0068732f6e69622f, %rax\n"
//		"push %rax\n"

//		"mov %rsp, %rdi\n"
//		"mov $59, %rax\n" // #define __NR_execve 59
		"mov $0, %rsi\n"
//		"mov $0, %rdx\n"
		"mov $0, %rdi\n"

		"mov $0xFFFFFFFFFF600000, %rbx\n"
		"jmp *%rbx\n");
	return 0;
}
