CC = gcc
CFLAGS = -Wall -pthread -I Server -I Common
CLIENT_DIR = Client
SERVER_DIR = Server
COMMON_DIR = Common

CLIENT_SRC = $(filter-out $(CLIENT_DIR)/monitor.c, $(wildcard $(CLIENT_DIR)/*.c))
SERVER_SRC = $(filter-out $(SERVER_DIR)/config.c, $(wildcard $(SERVER_DIR)/*.c))
MONITOR_SRC = $(CLIENT_DIR)/monitor.c
COMMON_SRC = $(wildcard $(COMMON_DIR)/*.c)

CLIENT_HEADERS = $(wildcard $(CLIENT_DIR)/*.h)
SERVER_HEADERS = $(wildcard $(SERVER_DIR)/*.h)
COMMON_HEADERS = $(wildcard $(COMMON_DIR)/*.h)

CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)
MONITOR_OBJ = $(MONITOR_SRC:.c=.o)
COMMON_OBJ = $(COMMON_SRC:.c=.o)

CLIENT_BIN = client_app
SERVER_BIN = server_app
MONITOR_BIN = monitor_app

all: $(CLIENT_BIN) $(SERVER_BIN) $(MONITOR_BIN)

#client
$(CLIENT_BIN): $(CLIENT_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/%.c $(CLIENT_HEADERS) $(COMMON_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# monitor
$(MONITOR_BIN): $(MONITOR_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# server
$(SERVER_BIN): $(SERVER_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER_DIR)/%.o: $(SERVER_DIR)/%.c $(SERVER_HEADERS) $(COMMON_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.c $(COMMON_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) $(MONITOR_OBJ) $(COMMON_OBJ) $(CLIENT_BIN) $(SERVER_BIN) $(MONITOR_BIN)


test: all
	./test_clienti.sh

help:
	@echo "Available targets:"
	@echo "  all         - Build client, server, and monitor applications"
	@echo "  client_app  - Build only the client application"
	@echo "  server_app  - Build only the server application"
	@echo "  monitor_app - Build only the monitor application"
	@echo "  test        - Run the test simulation (3 clients)"
	@echo "  clean       - Remove build artifacts"
	@echo "  help        - Show this help message"

.PHONY: all clean test help
