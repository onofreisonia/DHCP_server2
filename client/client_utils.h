#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <sys/un.h>
#include <arpa/inet.h>
#include "config.h"

// Keys for IPC
#define SHM_KEY 12345
#define SEM_KEY 54321

// Structure for Shared Memory
typedef struct {
    char current_ip[16];
    int has_ip;
    int lease_time;
    
} ClientState;

// Initializeaza socket-ul UDP/Unix
int create_socket();

// Configureaza adresa clientului si face bind
// Returneaza 0 la succes, -1 la eroare
int setup_client(int sockfd, struct sockaddr_un *my_addr);

// Configureaza adresa serverului
void setup_server_addr(struct sockaddr_un *server_addr);

// Helper pentru sters fisierul socket
void cleanup_socket(int sockfd, const char *path);

#endif
