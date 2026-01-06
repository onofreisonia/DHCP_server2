CC = gcc
CFLAGS = -Wall -pthread -I Server 
CLIENT_DIR = Client
SERVER_DIR = Server
CLIENT_SRC = $(wildcard $(CLIENT_DIR)/*.c)
SERVER_SRC = $(wildcard $(SERVER_DIR)/*.c)
CLIENT_HEADERS = $(wildcard $(CLIENT_DIR)/*.h)
SERVER_HEADERS = $(wildcard $(SERVER_DIR)/*.h)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_BIN = client_app
SERVER_BIN = server_app

all: $(CLIENT_BIN) $(SERVER_BIN)

#client
$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/%.c $(CLIENT_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# server
$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER_DIR)/%.o: $(SERVER_DIR)/%.c $(SERVER_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) $(CLIENT_BIN) $(SERVER_BIN)

.PHONY: all clean 
