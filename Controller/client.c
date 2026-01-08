#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/msg.h> 
#include <sys/shm.h> 
#include <sys/sem.h> 
#include <time.h>
#include "../Model/dhcp_message.h"
#include "../Service/client_utils.h"

int sockfd = -1;
struct sockaddr_in server_addr;
struct sockaddr_in my_addr;

char current_ip[16] = "";
int has_ip = 0;
int lease_time = 0;

pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;


//Utilizat pentru a trimite mesaje de la parinte la copil
int pipe_fd[2]; 
pid_t child_pid = 0;


//Utilizat pentru a trimite mesaje de la copil la parinte
int msg_queue_id = -1;


int shm_id = -1;
int sem_id = -1;
ClientState *shm_state = NULL; 

struct log_msg {
    long mtype;
    char mtext[256];
};


//Variabila globala pentru a opri thread-urile
volatile int running = 1;


//Thread-uri
void *input_thread_func(void *arg);
void *network_thread_func(void *arg);
void *logger_thread_func(void *arg); // Logger Thread
void renewal_child_process(int read_fd);

//Semaforuri
void sem_lock() {
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void sem_unlock() {
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}

//Update memorie partajata
void update_shm_state() {
    if (shm_state && sem_id != -1) {
        // Protectie dubla: Semafor Binar (1) + Mutex
        // Semaforul asigura accesul la SHM, Mutex-ul protejeaza structura interna
        sem_lock();
        pthread_mutex_lock(&shm_state->mutex);
        
        strcpy(shm_state->current_ip, current_ip);
        shm_state->has_ip = has_ip;
        shm_state->lease_time = lease_time;
        
        pthread_mutex_unlock(&shm_state->mutex);
        sem_unlock();
    }
}

void clean_exit() {
    if (sockfd != -1) {
        close(sockfd);
        cleanup_socket(-1, NULL);
        char marker_file[256];
        sprintf(marker_file, "Logs/client_sock_%d", getpid());
        unlink(marker_file);
    }
    //Sterge coada de mesaje
    if (msg_queue_id != -1) {
        msgctl(msg_queue_id, IPC_RMID, NULL);
    }
    
    //Sterge memoria partajata
    if (shm_state) shmdt(shm_state);
    
    //Sterge semaforul
    if (sem_id != -1) 
        semctl(sem_id, 0, IPC_RMID);
    if (shm_id != -1) 
        shmctl(shm_id, IPC_RMID, NULL);
    
}

#ifndef SIMULATE_ERRORS
#define SIMULATE_ERRORS 1
#endif

#define ERROR_RATE 20 // 20% sanse de eroare

int should_simulate_error() {
#ifdef SIMULATE_ERRORS
    if ((rand() % 100) < ERROR_RATE) {
        return 1;
    }
#endif
    return 0;
}

void handle_release() {
    pthread_mutex_lock(&state_mutex);
    if (has_ip) {
        DHCP_Message release_msg;
        memset(&release_msg, 0, sizeof(release_msg));
        release_msg.header.op = 1;
        release_msg.header.htype = 1;
        release_msg.header.hlen = 6;
        release_msg.header.xid = rand();
        release_msg.msg_type = DHCP_RELEASE;
        strcpy(release_msg.offered_ip, current_ip);

        sendto(sockfd, &release_msg, sizeof(DHCP_Message), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));
        
        printf("[CLIENT] DHCP RELEASE trimis pentru IP %s\n", current_ip);
        
        //Trimite semnalul de STOP catre copil
        write(pipe_fd[1], "STOP", 4);
        
        has_ip = 0;
        memset(current_ip, 0, sizeof(current_ip));
        
        update_shm_state(); //Actualizeaza memoria partajata
    } else {
        printf("[CLIENT] Nu am IP de eliberat.\n");
    }
    pthread_mutex_unlock(&state_mutex);
}

void handle_signal(int sig) {
    printf("\n[CLIENT] Intrerupere (Signal %d)...\n", sig);
    handle_release();
    clean_exit();
    exit(0);
}

int main() {
    srand(time(NULL) ^ getpid()); // Seed cu PID pentru unicitate in simulare multi-client

   //Gestionarea semnalelor
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Marker file for Monitor discovery (compatibility with existing monitor)
    char marker_file[256];
    sprintf(marker_file, "Logs/client_sock_%d", getpid());
    FILE *fp = fopen(marker_file, "w");
    if (fp) fclose(fp);

    //  Client & Server Init
    if ((sockfd = create_socket()) < 0) exit(EXIT_FAILURE);
    
    // Setare Timeout la receptie (pentru a nu bloca la simulare eroare)
    struct timeval tv;
    tv.tv_sec = 2;  // 2 secunde timeout
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Eroare setare timeout socket");
    }

    if (setup_client(sockfd, &my_addr) < 0) exit(EXIT_FAILURE);
    setup_server_addr(&server_addr);
    
    //Initializare Message Queue

    msg_queue_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msg_queue_id == -1) {
        perror("Eroare msgget");
        return 1;
    }
    
    
    // Setup memoria partajata si semaforul
    // Revenim la PID keys pentru a putea monitoriza mai multi clienti
    key_t shm_key = (key_t)getpid();
    key_t sem_key = (key_t)getpid();
    
    shm_id = shmget(shm_key, sizeof(ClientState), 0666 | IPC_CREAT);
    if (shm_id < 0) { perror("shmget"); return 1; }
    
    shm_state = (ClientState *)shmat(shm_id, NULL, 0);
    if (shm_state == (void *)-1) { perror("shmat"); return 1; }

    sem_id = semget(sem_key, 1, 0666 | IPC_CREAT);
    if (sem_id < 0) { perror("semget"); return 1; }
    
    
    //Initializare SEMAFOR cu valoarea 1 (Binar)
    semctl(sem_id, 0, SETVAL, 1);
    
    //Initializare MUTEX (Shared intre procese)
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shm_state->mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);
    
    //Initializare memoria partajata
    update_shm_state();

    
    //DHCP Handshake initializare
    printf("[CLIENT] Pornire DHCP handshake...\n");
    
    // DISCOVER
    DHCP_Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.op = 1;
    msg.header.htype = 1;
    msg.header.hlen = 6;
    msg.header.xid = rand();
    msg.msg_type = DHCP_DISCOVER;

    sendto(sockfd, &msg, sizeof(DHCP_Message), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("[CLIENT] DISCOVER trimis\n");

    // OFFER
    DHCP_Message offer;
    while(1) {
        int n = recvfrom(sockfd, &offer, sizeof(offer), 0, NULL, NULL);
        if (n < 0) {
            printf("[CLIENT] Timeout asteptare OFFER. Retrimit DISCOVER...\n");
            // Retrimitem DISCOVER
            msg.header.xid = rand();
            sendto(sockfd, &msg, sizeof(DHCP_Message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            continue;
        }
        
        // Simulare pierdere pachet
        if (should_simulate_error()) {
             printf("[SIMULARE] OFFER pierdut/ignorat. Astept retransmisie sau timeout...\n");
             continue;
        }

        if (offer.msg_type == DHCP_OFFER) {
            if (offer.header.xid != msg.header.xid) {
                // Ignore messages for other clients
                continue;
            }
            printf("[CLIENT] OFFER primit: IP=%s Lease=%d\n", offer.offered_ip, offer.lease_time);
            break; 
        } else {
             printf("[CLIENT] Mesaj ignorat (asteptam OFFER).\n");
        }
    }

    // REQUEST
    DHCP_Message req;
    memset(&req, 0, sizeof(req));
    req.header.xid = msg.header.xid;
    req.msg_type = DHCP_REQUEST;
    strcpy(req.offered_ip, offer.offered_ip);
    
    sendto(sockfd, &req, sizeof(req), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("[CLIENT] REQUEST trimis\n");

    // ACK
    DHCP_Message ack;
    while(1) {
        int n = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);
         if (n < 0) {
            printf("[CLIENT] Timeout asteptare ACK. Retrimit REQUEST...\n");
            // Retrimitem REQUEST
            sendto(sockfd, &req, sizeof(DHCP_Message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            continue;
        }

        if (should_simulate_error()) {
             printf("[SIMULARE] ACK pierdut/ignorat. Astept retransmisie sau timeout...\n");
             continue;
        }

        if (ack.msg_type == DHCP_ACK) {
             if (ack.header.xid != req.header.xid) {
                continue;
            }
            printf("[CLIENT] ACK primit. IP Confirmat: %s\n", ack.offered_ip);
            
            pthread_mutex_lock(&state_mutex);
            strcpy(current_ip, ack.offered_ip);
            lease_time = ack.lease_time;
            has_ip = 1;
            pthread_mutex_unlock(&state_mutex);
            
            update_shm_state();
            break; 
        } else if (ack.msg_type == DHCP_NAK) {
             if (ack.header.xid != req.header.xid) {
                continue;
            }
            printf("[CLIENT] NAK primit. Configurare esuata.\n");
            clean_exit();
            return 1;
        }
    }

    // Pipe
    if (pipe(pipe_fd) < 0) {
        perror("Pipe error");
        return 1;
    }

    
    //Fork pentru lease renewal
    child_pid = fork();
    if (child_pid == 0) {
       //Proces copil
        close(pipe_fd[1]); 
        renewal_child_process(pipe_fd[0]);
        exit(0);
    }
    
    // Proces Parinte
    close(pipe_fd[0]);

   
    //Creare threaduri pentru input, network si logging
    pthread_t input_th, net_th, log_th;
    
    if (pthread_create(&input_th, NULL, input_thread_func, NULL) != 0) {
        perror("Thread create failed");
    }
    if (pthread_create(&net_th, NULL, network_thread_func, NULL) != 0) {
        perror("Thread create failed");
    }
    //Logger thread
    if (pthread_create(&log_th, NULL, logger_thread_func, NULL) != 0) {
        perror("Thread create failed");
    }

  
    pthread_join(input_th, NULL);
    
    // Cleanup
    running = 0;
    
 
    struct log_msg quit_msg;
    quit_msg.mtype = 2; 
    strcpy(quit_msg.mtext, "QUIT");
    msgsnd(msg_queue_id, &quit_msg, sizeof(quit_msg.mtext), 0);
    pthread_join(log_th, NULL);

    pthread_detach(net_th); 
    kill(child_pid, SIGTERM);
    wait(NULL);
    
    clean_exit();
    return 0;
}


// Input (Interactive)
void *input_thread_func(void *arg) {
    char buffer[256];
    printf("\n[CLIENT-Input] Comenzi disponibile: status, release, exit\n");

    while (running) {
       
        
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) 
        {   // EOF (Background mode) -> Keep thread alive but idle
            while(running) sleep(1);
            break;
        }
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "status") == 0) {
            pthread_mutex_lock(&state_mutex);
            if (has_ip) {
                printf("[STATUS] IP: %s | Lease: %d sec\n", current_ip, lease_time);
            } else {
                printf("[STATUS] Niciun IP alocat.\n");
            }
            pthread_mutex_unlock(&state_mutex);
        }
        else if (strcmp(buffer, "release") == 0) {
            handle_release();
        }
        else if (strcmp(buffer, "exit") == 0) {
            handle_release();
            running = 0;
            break;
        }
        else {
            if (strlen(buffer) > 0)
                printf("Comanda necunoscuta.\n");
        }
    }
    return NULL;
}


// Network Receiver

void *network_thread_func(void *arg) {
    DHCP_Message msg;
    while (running) {
        int n = recvfrom(sockfd, &msg, sizeof(msg), 0, NULL, NULL);
        if (n <= 0) continue;

        // Simulare eroare (pieridere pachet la receptie)
        if (should_simulate_error()) {
            printf("\n[SIMULARE] Pachet pierdut/ignorat (Risc eroare %d%%)!\n", ERROR_RATE);
            continue;
        }

        if (msg.msg_type == DHCP_ACK) {
            
            pthread_mutex_lock(&state_mutex);
            int is_my_ip = (strcmp(msg.offered_ip, current_ip) == 0);
            if (is_my_ip) {
                 lease_time = msg.lease_time; 
            }
            pthread_mutex_unlock(&state_mutex);
            
            // Fix: Do not log ACKs for other clients to avoid confusion
            if (is_my_ip) {
                printf("\n[CLIENT-Net] ACK primit (Renew/Inform). IP: %s\n", msg.offered_ip);
                update_shm_state(); 
            }

        } else if (msg.msg_type == DHCP_NAK) {
            printf("\n[CLIENT-Net] NAK primit! IP invalidat.\n");
            
            pthread_mutex_lock(&state_mutex);
            has_ip = 0;
            memset(current_ip, 0, sizeof(current_ip));
            write(pipe_fd[1], "STOP", 4);
            pthread_mutex_unlock(&state_mutex);
            
            update_shm_state();
        }
    }
    return NULL;
}


// Logger 

void *logger_thread_func(void *arg) {
    struct log_msg msg;
    while (running) {
        // msgrcv este blocant
        if (msgrcv(msg_queue_id, &msg, sizeof(msg.mtext), 0, 0) == -1) {
            if (!running) break;
            perror("msgrcv");
            break;
        }
        
        if (strcmp(msg.mtext, "QUIT") == 0) {
            break;
        }

        printf("[CLIENT-Logger] %s\n", msg.mtext);
    }
    return NULL;
}


// Lease Renewal 

void renewal_child_process(int read_fd) {
    // Trimitem log-uri prin Message Queue
    struct log_msg msg;
    msg.mtype = 1;
    sprintf(msg.mtext, "Copil (PID %d) a pornit. Monitorizeaza timpul de leasing...", getpid());
    msgsnd(msg_queue_id, &msg, sizeof(msg.mtext), 0);

    while (1) {
        int sleep_time = 10; 
        
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(read_fd, &fds);
        
        struct timeval tv;
        tv.tv_sec = sleep_time;
        tv.tv_usec = 0;

        int ret = select(read_fd + 1, &fds, NULL, NULL, &tv);
        
        if (ret > 0) {
            //ceva pe pipe -> STOP
            memset(&msg, 0, sizeof(msg));
            msg.mtype = 1;
            strcpy(msg.mtext, "Semnal STOP via Pipe. Copilul iese.");
            msgsnd(msg_queue_id, &msg, sizeof(msg.mtext), 0);
            break;
        } else if (ret == 0) {
            // Timeout -> RENEW
            memset(&msg, 0, sizeof(msg));
            msg.mtype = 1;
            strcpy(msg.mtext, "Half-time lease a expirat! Trimitem RENEW request...");
            msgsnd(msg_queue_id, &msg, sizeof(msg.mtext), 0);
            
            if (strlen(current_ip) > 0) {
                DHCP_Message renew_req;
                memset(&renew_req, 0, sizeof(renew_req));
                renew_req.header.op = 1; 
                renew_req.header.htype = 1;
                renew_req.header.hlen = 6;
                renew_req.header.xid = rand();
                renew_req.msg_type = DHCP_REQUEST;
                strcpy(renew_req.offered_ip, current_ip);
                
                sendto(sockfd, &renew_req, sizeof(renew_req), 0, 
                       (struct sockaddr *)&server_addr, sizeof(server_addr));
            }
        }
    }
}