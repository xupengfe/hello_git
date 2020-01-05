// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <immintrin.h>

void sigaction_handler(int signum, siginfo_t *info, void *ptr)
{
	printf("For SIGUSR1:10.\nReceived signal:%d\n",
		info->si_signo);
	printf("si_errno:%d\nsi_code:%d\n", info->si_errno, info->si_code);
}

int signal_access_func(void)
{
	struct sigaction sigact;

	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_SIGINFO;
	//sigact.sa_flags = SA_ONSTACK; // one stack, not receive signal
	sigact.sa_sigaction = sigaction_handler;
	//sigact.sa_flags = SA_ONSTACK;
	sigaction(SIGUSR1, &sigact, NULL);
	raise(SIGUSR1);
	raise(SIGUSR1);
	raise(SIGUSR1);
	sigemptyset(&sigact.sa_mask);

	printf("After signal SIGUSR1:10\n");
	return 0;
}

int hack(void)
{
	printf("Access hack function, Which should be #cp blocked\n");
	exit (1);
}

void sigusr2_handler(int signum, siginfo_t *si, void *uc)
{
	int exp_sig=12;

	printf("For SIGUSR2:12.\nReceive signal signal:%d\n",
		si->si_signo);
	printf("si_errno:%d\nsi_code:%d\n", si->si_errno, si->si_code);

	if (si->si_signo != exp_sig) {
		printf("si_signo error, actual:%d, expect:%d.\n",
			si->si_signo, exp_sig);
		exit (1);
	}
}

int signal_sigusr2(void)
{
	int result;
	struct sigaction sa;

	result=sigemptyset(&sa.sa_mask);
	if (result) {
		printf("Init empty sa signal failed\n");
		return 2;
	}
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = sigusr2_handler;
	result = sigaction(SIGUSR2, &sa, NULL);
	if (result) {
		printf("Could not handle SIGUSR2(12)\n");
		return 2;
	}
	raise(SIGUSR2);
	raise(SIGUSR2);
}

void usage(void)
{
	printf("Usage: [a][s]\n");
	printf("  a: signal stack access cet stack check test\n");
	printf("  s: signal stack access and shstk violation test\n");
}

int main(int argc, char **argv)
{
	int result;
	char parm;

	if (argc == 1) {
		usage();
		exit(2);
	}
	else {
		sscanf(argv[1], "%c", &parm);
		printf("parm:%c\n",parm);
	}


	switch (parm) {
		case 'a':
			signal_access_func();
			break;
		case 's':
			signal_sigusr2();
			break;
		default:
			usage();
			exit (2);
	}

	return 0;
}
