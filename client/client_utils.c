#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "client_utils.h"

int create_socket() {
    int sockfd;
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("Eroare la crearea socketului");
        return -1;
    }
    return sockfd;
}

int setup_client(int sockfd, struct sockaddr_un *my_addr) {
    memset(my_addr, 0, sizeof(*my_addr));
    my_addr->sun_family = AF_UNIX;
    sprintf(my_addr->sun_path, CLIENT_PATH_TEMPLATE, getpid());

    unlink(my_addr->sun_path); // Sterge daca exista
    if (bind(sockfd, (struct sockaddr *)my_addr, sizeof(*my_addr)) < 0) {
        perror("Eroare la bind client");
        return -1;
    }
    return 0;
}

void setup_server_addr(struct sockaddr_un *server_addr) {
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sun_family = AF_UNIX;
    strcpy(server_addr->sun_path, SERVER_PATH);
}

void cleanup_socket(int sockfd, const char *path) {
    if (sockfd >= 0) {
        close(sockfd);
    }
    if (path && strlen(path) > 0) {
        unlink(path);
    }
}
