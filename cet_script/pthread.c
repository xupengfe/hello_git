// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * pthread test
 */
#include <stdio.h>
#include <pthread.h>

static void *run(void)
{
	int i = 0;

	while (i < 10) {
		i++;
		printf("pthread id = %ld\n", pthread_self());
		if (i == 5)
			pthread_exit(i); // means return i;
	}
	return NULL;
}

int main(void)
{
	int ret;
	pthread_t tid;
	void *val;

	ret = pthread_create(&tid, NULL, run, NULL);
	if (ret) {
		printf("pthread create error\n");
		return 1;
	}
	pthread_join(tid, &val);

	printf("val:%ls\n", (int *)val);

	return 0;
}
