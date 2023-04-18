#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdarg.h>

// Author - Nikita Koliada
// LOGIN - xkolia00

#define SEM_COUNT 8

#define SEMAPHORE_CUSTOMER "semaphore_customer"
#define SEMAPHORE_CLERC "semaphore_clerc"
#define SEMAPHORE_WFILE "semaphore_wfile"
#define SEMAPHORE_QUEUE "semaphore_queue"


#define FK "main.c"
#define FILENAME "proj2.out"
//struct for shared memory
typedef struct
{
    unsigned line_n; // for file iterations

    // int queue[3] = {0, 5, 0};
    //0 is no Z in queue, 5 is Z's id
    //[0] - 1, [1] - 2, [2] - 3  services
    int queue[3];
    int customer_n;
    int clerks_n;
    bool is_post_opened;
    unsigned shmid;
} ipc_t;

typedef struct
{
    //process id
    pid_t pid;
    //id of clerc or customer
    unsigned id;
    char type;
} process_t;



//creating shared memory
ipc_t* ipc_init(){
    key_t key = ftok(FK,'c');
    int ipc_key = shmget(key, sizeof(ipc_t), 0666 | IPC_CREAT);
    ipc_t* ipc = shmat(ipc_key, NULL, 0);
    ipc->shmid = ipc_key;
    ipc->line_n = 0;
    ipc->customer_n = 0;
    ipc->clerks_n = 0;
    ipc->queue[0] = 0;
    ipc->queue[2] = 0;
    ipc->queue[1] = 0;
    ipc->is_post_opened = true;
    return ipc;
}

//destroying shared memory
void ipc_destroy(ipc_t *ipc){
    key_t key = ipc->shmid;
    shmctl(key, IPC_RMID, NULL);
    shmdt(ipc);
}

//creating a process where type has 3 vars - 'm', 'u', 'z'
//m - main process, u - clerc, z - customer
process_t create_process(unsigned pid, unsigned id, char type){
    return ((process_t){pid, id, type});
}

bool no_queue(ipc_t *ipc){
    if(ipc->queue[0] == 0 && ipc->queue[1] == 0 && ipc->queue[2] == 0){
        return true;
    }
    else
        return false;
}

void print_msg(FILE *file, sem_t *sem_file, const char *format, ...) {
    sem_wait(sem_file);

    va_list args;
    va_start(args, format);

    vfprintf(file, format, args);
    fflush(file);

    va_end(args);
    sem_post(sem_file);

}

//void print_msg( sem_t *sem_file, const char *format, ...) {
//    FILE *file = fopen(FILENAME, "a");
//    if (file == NULL) {
//        perror("fopen");
//        exit(EXIT_FAILURE);
//    }
//    va_list args;
//    va_start(args, format);
//    sem_wait(sem_file);
//
//    vfprintf(file, format, args);
//    fflush(file);
//
//    sem_post(sem_file);
//    va_end(args);
//
//    fclose(file);
//
//}
void destroy_semaphores(sem_t **sem_array) {
    int i;
    for (i = 0; i < SEM_COUNT; i++) {
        if (sem_array[i] != NULL) {
            if (sem_close(sem_array[i]) != 0) {
                perror("sem_close");
            }
            sem_array[i] = NULL;
        }
    }
}
