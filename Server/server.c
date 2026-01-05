#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <semaphore.h>
#include "config.h"
#include "dhcp_message.h"
#define POOL_SIZE 100
#define MAX_THREADS 5  // Limitex nr de thread-uri active

pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER; // pt protejarea pool-ului de IP-uri

sem_t thread_sem; // pt limitarea thred-urilor

// Structura pentru a pasa argumente thread-ului
typedef struct 
{
    int sockfd;
    struct sockaddr_un client_addr;
    socklen_t addr_len;
    DHCP_Message msg;
    DHCP_ipconfig *config;
    IP_Entry *ip_pool;
} ThreadArgs;

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
        break;
    case DHCP_REQUEST:
        printf("[THREAD %lu] REQUEST primit\n", pthread_self());
        handle_dhcp_request(data->sockfd, (struct sockaddr *)&data->client_addr, data->addr_len,
                            &data->msg, data->config, data->ip_pool, POOL_SIZE);
        break;
    case DHCP_RELEASE:
        printf("[THREAD %lu] RELEASE primit\n", pthread_self());
        handle_dhcp_release(&data->msg, data->ip_pool, POOL_SIZE);
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
    DHCP_ipconfig configuratie;
    
    if (load_config("Server/ipconfig.txt", &configuratie) == -1) 
    {
        // daca nu gaseste calea relativa, incercam direct
        if (load_config("ipconfig.txt", &configuratie) == -1) 
        {
            fprintf(stderr, "Nu s-a incarcat configuratia serverului\n");
            return 1;
        }
    }
    printf("Server DHCP pornit (Unix Sockets & Multithreading).\n");
    int sockfd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        perror("Eroare la crearea socketului server");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SERVER_PATH, sizeof(server_addr.sun_path) - 1);
    unlink(SERVER_PATH);
    

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Eroare la bind server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Server asculta pe socket-ul: %s\n", SERVER_PATH);

    if (sem_init(&thread_sem, 0, MAX_THREADS) != 0) {
        perror("Eroare initializare semafor");
        return 1;
    }
    

    IP_Entry ip_pool[POOL_SIZE];
    struct in_addr base_ip;
    inet_aton(configuratie.range_start, &base_ip);
    for (int i = 0; i < POOL_SIZE; i++)
    {
        ip_pool[i].allocated = 0;
        ip_pool[i].lease_start = 0;
        ip_pool[i].lease_time = 0;
        struct in_addr tmp;
        tmp.s_addr = htonl(ntohl(base_ip.s_addr) + i);
        strcpy(ip_pool[i].ip, inet_ntoa(tmp));
    }
    while (1)
    { 
        sem_wait(&thread_sem);
        DHCP_Message msg_recv;
        
        // Receptionam mesajul
        int n = recvfrom(sockfd, &msg_recv, sizeof(msg_recv), 0,
                         (struct sockaddr *)&client_addr, &addr_len);
        if (n < sizeof(DHCP_Message)) 
        {
            sem_post(&thread_sem); // Daca a fost eroare sau mesaj scurt, eliberam semaforul si continuam
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
    close(sockfd);
    unlink(SERVER_PATH);
    sem_destroy(&thread_sem);
    pthread_mutex_destroy(&pool_mutex);
    return 0;
}