#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include "client_utils.h"


//Semaforul e folosit pentru a proteja accesul la memoria partajata
void sem_lock(int sem_id) {
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void sem_unlock(int sem_id) {
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}

int main() {
    //Memoria partajata e folosita pentru a comunica cu serverul

    int shm_id = shmget(SHM_KEY, sizeof(ClientState), 0666);
    if (shm_id < 0) {
        printf("[MONITOR] Clientul nu pare sa ruleze (SHM lipseste).\n");
        return 1;
    }

    ClientState *state = (ClientState *)shmat(shm_id, NULL, 0);
    if (state == (void *)-1) {
        perror("shmat");
        return 1;
    }

    //Semaforul e folosit pentru a proteja accesul la memoria partajata
    int sem_id = semget(SEM_KEY, 1, 0666);
    if (sem_id < 0) {
        printf("[MONITOR] Semaforul lipseste.\n");
        return 1;
    }

    printf("[MONITOR] Atasat la memoria partajata. Pornire monitorizare...\n");
    printf("Apasa Ctrl+C pentru iesire.\n\n");

    while (1) {
        sem_lock(sem_id);
        
        //Citirea datelor protejate
        if (state->has_ip) {
            printf("\r[MONITOR] Status: CONECTAT | IP: %-15s | Lease: %d s   ", 
                   state->current_ip, state->lease_time);
        } else {
            printf("\r[MONITOR] Status: FARA IP                                 ");
        }
        fflush(stdout);

        sem_unlock(sem_id);
        
        sleep(1);
    }

    shmdt(state);
    return 0;
}
