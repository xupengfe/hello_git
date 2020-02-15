BIN = t1 t3 test_vsyscall sigsetjump

all: $(BIN)

$(all):
	$(CC) -o $@ $<

test_vsyscall:
	$(CC) -m64 -o test_vsyscall test_vsyscall.c -ldl

clean:
	rm -f $(BIN)
