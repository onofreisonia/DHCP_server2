CC = gcc
CFLAGS = -Wall -g -pthread
LDFLAGS = 

SRCS = client.c dhcp_message.c client_utils.c
OBJS = $(SRCS:.c=.o)

all: client_app monitor

client_app: $(OBJS)
	$(CC) $(CFLAGS) -o client_app $(OBJS)

monitor: monitor.c client_utils.c
	$(CC) $(CFLAGS) -o monitor monitor.c client_utils.c

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) client_app monitor

.PHONY: all clean
