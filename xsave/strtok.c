#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	char str[]="ab|cd|ef=y";
	char *ptr;
	char *p;
	char *left;

	printf("before strtok:  str=%s\n",str);
	printf("after:\n");
	ptr = strtok_r(str, "|=", &p);
	while(ptr != NULL){
		//printf("str:%s\n",str);
		printf("ptr:%s\n",ptr);
		printf("p:%s\n\n",p);
		ptr = strtok_r(NULL, "|=", &p);
		//left = strchr(p, '=');
		//if (!left)
		//	break;
		if (strlen(p) == 0)
			break;
	}
	return 0;
}
