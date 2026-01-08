#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "client_utils.h"

int create_socket() {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Eroare la crearea socketului");
        return -1;
    }
    
    int broadcast = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("Eroare setsockopt BROADCAST");
        return -1;
    }

    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Eroare setsockopt REUSEADDR");
        return -1;
    }

    return sockfd;
}

int setup_client(int sockfd, struct sockaddr_in *my_addr) {
    memset(my_addr, 0, sizeof(*my_addr));
    my_addr->sin_family = AF_INET;
    my_addr->sin_port = htons(CLIENT_PORT);
    my_addr->sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)my_addr, sizeof(*my_addr)) < 0) {
        perror("Eroare la bind client");
        return -1;
    }
    return 0;
}

void setup_server_addr(struct sockaddr_in *server_addr) {
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(SERVER_PORT);
    server_addr->sin_addr.s_addr = htonl(INADDR_BROADCAST); // Trimitem la Broadcast
}

void cleanup_socket(int sockfd, const char *path) {
    if (sockfd >= 0) {
        close(sockfd);
    }
    // Nu mai stergem fisiere
}
