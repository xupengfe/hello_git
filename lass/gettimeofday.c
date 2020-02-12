#include <stdio.h>

int main(void)
{
	/*
	* gettimeofday() takes two arguments, if these are null,
	* the syscall will return without filling these
	arguments.
	* As per the SystemV AMD64 ABI
	* convention, the first and second arguments are passed
	in
	* %rdi and %rsi.
	**
	gettimeofday() is located at 0xffffffffff600000
	*/
	__asm__("mov $0, %rdi\n"
		"mov $0, %rsi\n"
		"mov 0xffffffffff600000, %rax\n"
		"call *%rax\n"
		:
		:
		:"rax", "rdi", "rsi");

	return 0;
}