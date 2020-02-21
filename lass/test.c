// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int backdoor(void)
{
	printf("Access backdoor, protect failed\n");
	exit(2);
}

int main(void)
{
	unsigned long *j;

	j = __builtin_frame_address(0);
	*(j+1) = (unsigned long)backdoor;

	asm(
		"mov $0, %rsi\n"
		"mov $0, %rdi\n"

		"mov $0xFFFFFFFFFF600000, %rbx\n"
		"jmp *%rbx\n"
		"ret");
	return 0;
}
