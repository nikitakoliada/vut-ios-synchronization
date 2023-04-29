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

#define SEM_COUNT 6

#define SEMAPHORE_QUEUE "semaphore_queue"
#define MUTEX_POST "semaphore_post"
#define SEMAPHORE_WFILE "semaphore_wfile"
#define MUTEX_QUEUE "mutex_queue"


#define FK "proj2.c"
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
    bool status_post;
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
    ipc->status_post = true;
    return ipc;
}

//destroying shared memory
void ipc_destroy(ipc_t *ipc){
    key_t key = ipc->shmid;
    shmctl(key, IPC_RMID, NULL);
    shmdt(ipc);
}

//creating a process where type has 3 vars - 'm', 'u', 'z'
//m - main process, u - clerk, z - customer
process_t create_process(unsigned pid, unsigned id, char type){
    return ((process_t){pid, id, type});
}

//check if no customers in queue
bool no_queue(sem_t *sem, ipc_t *ipc){
    //waiting for the semaphore( no simultaneous access possible)
    sem_wait(sem);
    if(ipc->queue[0] == 0 && ipc->queue[1] == 0 && ipc->queue[2] == 0){
        sem_post(sem);
        return true;
    }
    else {
        sem_post(sem);
        return false;
    }
}

//print the message to the file
void print_msg(FILE *file, const char *format, ...) {

    va_list args;
    va_start(args, format);

    vfprintf(file, format, args);
    fflush(file);

    va_end(args);

}

//destroy all semaphores
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

//unlink all the semaphores
void unlink_semaphores(){
    for (int i = 0; i < 3; i++) {
        char name[50];
        sprintf(name, "%s%d", SEMAPHORE_QUEUE, i);
        sem_unlink(name);
    }
    sem_unlink(MUTEX_QUEUE);
    sem_unlink(SEMAPHORE_WFILE);
    sem_unlink(MUTEX_POST);
}

