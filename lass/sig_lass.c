// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

void segv_handler(int signum, siginfo_t *si, void *uc)
{
    printf("sig:%d, no:%d, err:%d, si_code:%d\n",
        signum, si->si_signo, si->si_errno, si->si_code);

    exit(0);
}

int main(void)
{
    int r, hack_a;
    struct sigaction sa;

    r = sigemptyset(&sa.sa_mask);
    if (r) {
        printf("Init empty signal failed\n");
    return -1;
    }
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = segv_handler;
    r = sigaction(SIGSEGV, &sa, NULL);
    if (r) {
        printf("Could not handle SIGSEGV(11)\n");
        return -1;
    }

    hack_a = *(const int *)0xffffffffff600000;

}
