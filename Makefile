CC = gcc
CFLAGS = -Wall -g -pthread -I./Model -I./Service -I./View -I./Common

# Directories
CONTROLLER_DIR = Controller
SERVICE_DIR = Service
VIEW_DIR = View
MODEL_DIR = Model

# Targets
all: server client monitor

server: $(CONTROLLER_DIR)/server.c $(SERVICE_DIR)/dhcp_message.c $(SERVICE_DIR)/config.c
	$(CC) $(CFLAGS) -o server $(CONTROLLER_DIR)/server.c $(SERVICE_DIR)/dhcp_message.c $(SERVICE_DIR)/config.c

client: $(CONTROLLER_DIR)/client.c $(SERVICE_DIR)/client_utils.c
	$(CC) $(CFLAGS) -o client_app $(CONTROLLER_DIR)/client.c $(SERVICE_DIR)/client_utils.c

monitor: $(VIEW_DIR)/monitor.c $(SERVICE_DIR)/client_utils.c
	$(CC) $(CFLAGS) -o monitor $(VIEW_DIR)/monitor.c $(SERVICE_DIR)/client_utils.c

clean:
	rm -f server client_app monitor *.o *.log Logs/*.log

.PHONY: all clean
