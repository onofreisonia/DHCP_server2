#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "config.h"

// Keys for IPC
#define SHM_KEY 12345
#define SEM_KEY 54321

// Structure for Shared Memory
typedef struct {
    char current_ip[16];
    int has_ip;
    int lease_time;
    pthread_mutex_t mutex;
} ClientState;

// Initializeaza socket-ul UDP/INET
int create_socket();

// Configureaza adresa clientului si face bind
// Returneaza 0 la succes, -1 la eroare
int setup_client(int sockfd, struct sockaddr_in *my_addr);

// Configureaza adresa serverului
void setup_server_addr(struct sockaddr_in *server_addr);

// Helper pentru sters fisierul socket (pastrat pt compatibilitate, dar gol)
void cleanup_socket(int sockfd, const char *path);

#endif
