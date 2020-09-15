#include ../make.inc
CC  = gcc
CFLAGS = -fcf-protection=full -mshstk -O0 -fno-stack-check -fno-stack-protector
LFLAGS = -mshstk -fno-stack-check -fno-stack-protector
SFLAGS = -fcf-protection=full -mshstk -fno-stack-check -fno-stack-protector

BIN = pthread

all: $(BIN)

$(all):
	$(CC) $(CFLAGS) -o $@ $<

pthread:
	$(CC) -pthread -o pthread pthread.c

clean:
	rm -f $(BIN)
