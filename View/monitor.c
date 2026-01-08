#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <pthread.h>
#include "client_utils.h"

// Helper pentru semafor (Reader - consuma 1 resursa din 3)
void sem_lock_reader(int sem_id) {
    if (sem_id < 0) return;
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void sem_unlock_reader(int sem_id) {
    if (sem_id < 0) return;
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}

// Functie pentru a verifica daca un sir e numar
int is_number(const char *s) {
    while (*s) {
        if (*s < '0' || *s > '9') return 0;
        s++;
    }
    return 1;
}

int main() {
    printf("[MONITOR] Multi-Client Monitor Pornit (SHM: Sem=3, Mutex).\n");
    printf("Cautare clienti activi (dupa fisierele socket client_sock_PID)...\n");
    printf("Apasa Ctrl+C pentru iesire.\n\n");

    while (1) {
        // Curatam ecranul (ANSI escape code)
        printf("\033[H\033[J");
        
        printf("==============================================================\n");
        printf("| %-8s | %-15s | %-6s | %-6s |\n", "PID", "IP Address", "Lease", "Status");
        printf("==============================================================\n");

        DIR *d;
        struct dirent *dir;
        d = opendir("Logs");
        int found = 0;

        if (d) {
            while ((dir = readdir(d)) != NULL) {
                // Cautam fisiere de forma "client_sock_<PID>"
                if (strncmp(dir->d_name, "client_sock_", 12) == 0) {
                    char *pid_str = dir->d_name + 12;
                    if (is_number(pid_str)) {
                        int pid = atoi(pid_str);
                        
                        // Incercam sa ne atasam la SHM folosind PID-ul
                        int shm_id = shmget((key_t)pid, sizeof(ClientState), 0666);
                        int sem_id = semget((key_t)pid, 1, 0666);

                        if (shm_id >= 0 && sem_id >= 0) {
                            ClientState *state = (ClientState *)shmat(shm_id, NULL, 0);
                            if (state != (void *)-1) {
                                // 1. Luam un loc la masa (Semafor Binar 1)
                                sem_lock_reader(sem_id);
                                
                                // 2. Intram in zona critica protejata de Mutex pentru citire
                                pthread_mutex_lock(&state->mutex);
                                
                                char status[10];
                                if (state->has_ip) strcpy(status, "BOUND");
                                else strcpy(status, "INIT");

                                printf("| %-8d | %-15s | %-6d | %-6s |\n", 
                                       pid, 
                                       state->has_ip ? state->current_ip : "-", 
                                       state->lease_time,
                                       status);
                                       
                                // 3. Iesim din zona critica
                                pthread_mutex_unlock(&state->mutex);
                                
                                // 4. Eliberam locul la masa
                                sem_unlock_reader(sem_id);
                                
                                shmdt(state);
                                found++;
                            }
                        } else {
                            // Procesul poate a murit dar socket-ul a ramas, sau invers
                            printf("| %-8d | %-15s | %-6s | %-6s |\n", pid, "?", "?", "ZOMBIE");
                        }
                    }
                }
            }
            closedir(d);
        }
        
        printf("==============================================================\n");
        if (found == 0) {
            printf("[INFO] Niciun client detectat.\n");
        } else {
            printf("[INFO] %d clienti activi detectati.\n", found);
        }
        
        sleep(1);
    }
    return 0;
}