BIN = ax cpuid cpu_feature

all: $(BIN)

$(all):
	$(CC) -o $@ $<

clean:
	rm -f $(BIN)
