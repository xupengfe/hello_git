/*
 * cpuid.c
 *
 * This file show reuqested cpuid info and verify the result is correct.
 *
 * Copyright (C) 2017, Intel - http://www.intel.com/
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * Contributors:
 *      Legacy cpuid.c author and contributors
 *      - Inital implementation, show cpuid
 *      Pengfei Xu (Pengfei.Xu@intel.com)
 *      - Add the cpuid binary info show
 *      - Check cpu funtion bit is set correctly
 *
*/

/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define N 32
#define M 40

// cpuid check function with asm
static inline void native_cpuid(unsigned int *eax, unsigned int *ebx,
        unsigned int *ecx, unsigned int *edx)
{
  /* ecx is often an input as well as an output. */
  asm volatile("cpuid"
  : "=a" (*eax),
  "=b" (*ebx),
  "=c" (*ecx),
  "=d" (*edx)
  : "0" (*eax), "2" (*ecx)
  : "memory");
}

// Convert hex to binary to show cpuid info
int h_to_b(long int n)
{
  int i=0;
  static int a[N]; //avoid clean function, return error
  for (i=0; i != N; ++i)
  {
    a[N-1-i] = n % 2;
    n /= 2;
  }
  for (i=0; i != N; ++i)
  {
    printf("%d",a[i]);
    if((i+1)%4 == 0)
      printf(" ");
  }
  printf("\n");
  return 0;
}

// Check request cpuid is correct or not
int check_id(long int n, int ex_number)
{
  int i=0;
  static int b[N]; //avoid clean function, return error
  int bit_n=N-1-ex_number;
  for (i=0; i != N; ++i)
  {
    b[N-1-i] = n % 2;
    n /= 2;
  }
  printf("Start with 0, pass: bit set 1, fail: bit set 0\n");
  if (b[bit_n] == 1)
  {
    printf("Order bit:%d, invert order:%d, bit:%d, pass!\n", bit_n, ex_number, b[bit_n]);
    return 0;
  }
  else
  {
    printf("Order bit:%d, invert order:%d, bit:%d, fail!\n", bit_n, ex_number, b[bit_n]);
    return 1;
  }
}

int main(int argc, char *argv[])
{
  unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
  int ex_n,test_result=1;
  char ex='e';
  if (argc == 1)
  {
    eax=1;
    printf("No parameter! So use eax = 1 as default to execute!\n");
  }
  else if (argc == 5)
  {
    sscanf(argv[1], "%x", &eax);
    printf("4 parameters, eax=%d\n", eax);
    sscanf(argv[2], "%x", &ebx);
    sscanf(argv[3], "%x", &ecx);
    sscanf(argv[4], "%x", &edx);
  }
  else if (argc == 7)
  {
    sscanf(argv[1], "%x", &eax);
    printf("6 parameters, eax=%d\n", eax);
    sscanf(argv[2], "%x", &ebx);
    sscanf(argv[3], "%x", &ecx);
    sscanf(argv[4], "%x", &edx);
    sscanf(argv[5], "%c", &ex);
    sscanf(argv[6], "%d", &ex_n);
  }
  else
  {
    sscanf(argv[1], "%x", &eax);
    printf("Just get eax=%d\n", eax);
  }

  printf("cpuid(eax=%08x, ebx=%08x, ecx=%08x, edx=%08x)\n", \
          eax, ebx, ecx,edx);
  printf("cpuid(&eax=%p, &ebx=%p, &ecx=%p, &edx=%p)\n", \
          &eax, &ebx, &ecx, &edx);
  native_cpuid(&eax, &ebx, &ecx, &edx);
  printf("After native_cpuid:\n");
  printf("out:  eax=%08x, ebx=%08x, ecx=%08x,  edx=%08x\n", \
          eax, ebx, ecx, edx);
  printf("cpuid(&eax=%p, &ebx=%p, &ecx=%p, &edx=%p)\n", \
          &eax, &ebx, &ecx, &edx);
  printf("output:\n");
  printf("  eax=%08x    || Binary: ",eax);
  h_to_b(eax);
  printf("  ebx=%08x    || Binary: ",ebx);
  h_to_b(ebx);
  printf("  ecx=%08x    || Binary: ",ecx);
  h_to_b(ecx);
  printf("  edx=%08x    || Binary: ",edx);
  h_to_b(edx);

  printf("Now check cpuid e%cx, bit %d\n", ex, ex_n);
  if (ex == 'a')
    test_result=check_id(eax, ex_n);
  else if (ex == 'b')
    test_result=check_id(ebx, ex_n);
  else if (ex == 'c')
    test_result=check_id(ecx, ex_n);
  else if (ex == 'd')
    test_result=check_id(edx, ex_n);
  else
  {
    printf("No check point, not in a-d, skip.\n");
    test_result=0;
  }

  printf("Done! Result:%d.\n\n",test_result);
  return test_result;
}
