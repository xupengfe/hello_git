BIN = t1 t2 t3 t4 test test_vsyscall sigsetjump lass_test \
        clflush value sig_lass

all: $(BIN)

$(all):
	$(CC) -o $@ $<

test_vsyscall:
	$(CC) -m64 -o test_vsyscall test_vsyscall.c -ldl

clean:
	rm -f $(BIN)
