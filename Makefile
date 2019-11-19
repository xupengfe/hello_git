BIN = ax

all: $(BIN)

$(all):
	$(CC) -o $@ $<

clean:
	rm -f $(BIN)
