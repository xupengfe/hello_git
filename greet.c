#include<stdio.h>
#include <unistd.h>
#include <string.h>

void greeting()

{
	char *msg = "Hello, world!\n";

	write(1, msg, strlen(msg));
}



void main()

{
	greeting();
}
