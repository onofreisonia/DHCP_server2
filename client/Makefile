CC = gcc
CFLAGS = -Wall
SRC = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJ = $(SRC:.c=.o)
BIN = client_app

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
