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


#define SEMAPHORE_CUSTOMER "semaphore_customer"
#define SEMAPHORE_CLERC "semaphore_clerc"
#define SEMAPHORE_WFILE "semaphore_wfile"

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
    unsigned customer_n;
    unsigned clerks_n;
    bool is_post_opened;
    unsigned shmid;
} ipc_t;

typedef struct
{
    //process id
    unsigned pid;
    //id of clerc or customer
    unsigned id;
    char type;
} process_t;

void write_file(char *msg){

}

//creating shared memory
ipc_t* ipc_init(){
    key_t key = ftok(FK,'c');
    int ipc_key = shmget(key, sizeof(ipc_t), 0666 | IPC_CREAT);
    ipc_t* ipc = shmat(ipc_key, NULL, 0);
    ipc->shmid = ipc_key;
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
void print_msg(FILE *file, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);
}


//void print_msg(FILE *file, sem_t *sem_file, const char *format, ...) {
//    va_list args;
//    va_start(args, format);
//    unsigned *num = va_arg(args, unsigned *);
//    sem_wait(sem_file);
//
//    vfprintf(file, format, args);
//    num++;
//    sem_post(sem_file);
//
//    va_end(args);
//}