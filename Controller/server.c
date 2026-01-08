#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include "config.h"
#include "dhcp_message.h"

#define POOL_SIZE 100
#define MAX_THREADS 5
#define LEASES_FILE "leases.txt"

pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t thread_sem;
volatile int running = 1;

typedef struct 
{
    int sockfd;
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    DHCP_Message msg;
    DHCP_ipconfig *config;
    IP_Entry *ip_pool;
} ThreadArgs;

void handle_signal(int sig) {
    if (running) {
        printf("\n[SERVER] Semnal %d primit. Oprire server...\n", sig);
        running = 0;
        // Putem închide socketul aici dacă este global, sau lăsăm while-ul să se termine natural (cu timeout sau la urm pachet)
    }
}

void save_leases(IP_Entry *pool) {
    FILE *f = fopen(LEASES_FILE, "w");
    if (!f) {
        perror("[SERVER] Eroare la salvarea leases.txt");
        return;
    }
    for (int i = 0; i < POOL_SIZE; i++) {
        if (pool[i].allocated) {
            fprintf(f, "%s %ld %d\n", pool[i].ip, (long)pool[i].lease_start, pool[i].lease_time);
        }
    }
    fclose(f);
    printf("[SERVER] Leases salvate in %s (Text Format)\n", LEASES_FILE);
}

void load_leases(IP_Entry *pool) {
    FILE *f = fopen(LEASES_FILE, "r");
    if (!f) {
        printf("[SERVER] Nu exista leases.txt (sau eroare la deschidere). Se porneste cu pool gol.\n");
        return;
    }
    
    char ip[16];
    long start;
    int duration;
    int loaded = 0;

    while (fscanf(f, "%15s %ld %d", ip, &start, &duration) == 3) {
        for (int i = 0; i < POOL_SIZE; i++) {
            if (strcmp(pool[i].ip, ip) == 0) {
                pool[i].allocated = 1;
                pool[i].lease_start = (time_t)start;
                pool[i].lease_time = duration;
                loaded++;
                break;
            }
        }
    }
    fclose(f);
    printf("[SERVER] %d Leases incarcate din %s\n", loaded, LEASES_FILE);
}

void *lease_cleaner(void *arg) {
    IP_Entry *pool = (IP_Entry *)arg;
    printf("[CLEANER] Thread de curatare pornit.\n");

    while (running) {
        sleep(5); // Verificam la 5 secunde
        
        time_t now = time(NULL);
        pthread_mutex_lock(&pool_mutex);
        
        int cleaned = 0;
        for (int i = 0; i < POOL_SIZE; i++) {
            if (pool[i].allocated) {
                if (pool[i].lease_start + pool[i].lease_time < now) {
                    printf("[CLEANER] IP %s expirat. Eliberare automata.\n", pool[i].ip);
                    pool[i].allocated = 0;
                    pool[i].lease_start = 0;
                    pool[i].lease_time = 0;
                    cleaned++;
                }
            }
        }
        
        if (cleaned > 0) {
            save_leases(pool); // Salvam starea actualizata
        }
        
        pthread_mutex_unlock(&pool_mutex);
    }
    printf("[CLEANER] Oprire thread.\n");
    return NULL;
}

void *client_handler(void *arg) 
{
    ThreadArgs *data = (ThreadArgs *)arg;
    
    pthread_mutex_lock(&pool_mutex);
    switch (data->msg.msg_type)
    {
    case DHCP_DISCOVER:
        printf("[THREAD %lu] DISCOVER primit (xid=%u)\n", pthread_self(), data->msg.header.xid);
        handle_dhcp_discover(data->sockfd, (struct sockaddr *)&data->client_addr, data->addr_len,
                             &data->msg, data->config, data->ip_pool, POOL_SIZE);
        save_leases(data->ip_pool); // Salvare dupa modificare
        break;
    case DHCP_REQUEST:
        printf("[THREAD %lu] REQUEST primit\n", pthread_self());
        handle_dhcp_request(data->sockfd, (struct sockaddr *)&data->client_addr, data->addr_len,
                            &data->msg, data->config, data->ip_pool, POOL_SIZE);
        save_leases(data->ip_pool); // Salvare dupa modificare
        break;
    case DHCP_RELEASE:
        printf("[THREAD %lu] RELEASE primit\n", pthread_self());
        handle_dhcp_release(&data->msg, data->ip_pool, POOL_SIZE);
        save_leases(data->ip_pool); // Salvare dupa modificare
        break;
    case DHCP_INFORM:
        printf("[THREAD %lu] INFORM primit\n", pthread_self());
        send_dhcp_ack_inform(data->sockfd, (struct sockaddr *)&data->client_addr, data->addr_len,
                             &data->msg, data->config);
        break;
    default:
        printf("[THREAD %lu] Mesaj necunoscut: %d\n", pthread_self(), data->msg.msg_type);
        break;
    }
    pthread_mutex_unlock(&pool_mutex);
    free(data);
    
    sem_post(&thread_sem);
    
    return NULL;
}

int main()
{
    // Signal handling
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    DHCP_ipconfig configuratie;
    
    if (load_config("Model/ipconfig.txt", &configuratie) == -1) // Assuming running from root
    {
         if (load_config("ipconfig.txt", &configuratie) == -1) 
        {
            fprintf(stderr, "Nu s-a incarcat configuratia serverului\n");
            return 1;
        }
    }

    // IP Pool Init
    IP_Entry ip_pool[POOL_SIZE];
    struct in_addr base_ip;
    inet_aton(configuratie.range_start, &base_ip);
    
    // Initialize pool structures first
    for (int i = 0; i < POOL_SIZE; i++)
    {
        ip_pool[i].allocated = 0;
        ip_pool[i].lease_start = 0;
        ip_pool[i].lease_time = 0;
        struct in_addr tmp;
        tmp.s_addr = htonl(ntohl(base_ip.s_addr) + i);
        strcpy(ip_pool[i].ip, inet_ntoa(tmp));
    }

    // Load persisted state if available
    load_leases(ip_pool);

    printf("Server DHCP pornit (INET Sockets & Multithreading).\n");
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Eroare la crearea socketului server");
        exit(EXIT_FAILURE);
    }

    // Set socket timeout to allow checking 'running' flag periodically
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Eroare la bind server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Server asculta pe portul: %d\n", SERVER_PORT);

    if (sem_init(&thread_sem, 0, MAX_THREADS) != 0) {
        perror("Eroare initializare semafor");
        return 1;
    }
    
    // Start Lease Cleaner Thread
    pthread_t cleaner_tid;
    pthread_create(&cleaner_tid, NULL, lease_cleaner, (void *)ip_pool);

    while (running)
    { 
        sem_wait(&thread_sem);
        
        if (!running) {
             sem_post(&thread_sem);
             break;
        }

        DHCP_Message msg_recv;
        
        // Receptionam mesajul
        int n = recvfrom(sockfd, &msg_recv, sizeof(msg_recv), 0,
                         (struct sockaddr *)&client_addr, &addr_len);
                         
        if (n < 0) {
            // Timeout or error
            sem_post(&thread_sem);
            continue;
        }

        if (n < sizeof(DHCP_Message)) 
        {
            sem_post(&thread_sem); 
            continue;
        }

        ThreadArgs *args = (ThreadArgs *)malloc(sizeof(ThreadArgs));
        if (!args) {
            perror("Eroare alocare memorie args");
            sem_post(&thread_sem);
            continue;
        }
        args->sockfd = sockfd;
        args->client_addr = client_addr;
        args->addr_len = addr_len;
        args->msg = msg_recv;
        args->config = &configuratie;
        args->ip_pool = ip_pool;

        // Creez thread-ul
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, args) != 0) {
            perror("Eroare creare thread");
            free(args);
            sem_post(&thread_sem);
            continue;
        }
        // Detasam thread-ul pentru a nu fi nevoie de join
        pthread_detach(tid);
    }

    printf("[SERVER] Inchidere curata...\n");
    
    // Cleanup
    running = 0; // Ensure flag is set for cleaner thread
    pthread_join(cleaner_tid, NULL); // Wait for cleaner to finish

    save_leases(ip_pool); // Final save

    close(sockfd);
    // Unlink removed for INET sockets
    sem_destroy(&thread_sem);
    pthread_mutex_destroy(&pool_mutex);
    
    printf("[SERVER] La revedere!\n");
    return 0;
}