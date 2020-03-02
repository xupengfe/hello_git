BIN = ax cpuid cpu_feature hello hello_ins hello_sys

all: $(BIN)

$(all):
	$(CC) -o $@ $<

hello_ins:
	as -o hello_ins.o hello_ins.S
	ld -o hello_ins hello_ins.o

clean:
	rm -f $(BIN)
