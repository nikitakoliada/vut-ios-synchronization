#include "header.h"

// Author - Nikita Koliada
// LOGIN - xkolia00

int main(int argc, char* argv[]) {
    srand(time(NULL)); // initialize random number generator

    // parse command line arguments
    if (argc != 6) {
        fprintf(stderr, "Usage: %s NZ NU TZ TU F\n", argv[0]);
        return 1;

    }

    //checking the passed arguments
    for (int i = 1; i < argc; i++){
        //check if the arguments are numbers
        for (int j = 0; argv[i][j]; j++) {
            int n = argv[i][j];
            if (!(n >= '0' && n <= '9')) {
                fprintf(stderr, "The arguments must be positive numbers\n");
                return 1;
            }
        }
        if((i == 1 && atoi(argv[i]) < 0) || (i == 2 && atoi(argv[i]) <= 0)) {
            fprintf(stderr, "The 1 argument must be >= 0 and 2 argument must be > 0\n");
            return 1;
        }

        if(i == 3 && (atoi(argv[i]) > 10000 || atoi(argv[i]) < 0)) {
            fprintf(stderr, "The 3 argument must be >=0 and <=10000\n");
            return 1;
        }
        if(i == 4 && (atoi(argv[i]) > 100 || atoi(argv[i]) < 0)) {
            fprintf(stderr, "The 4 argument must be >=0 and <=100\n");
            return 1;
        }
        if(i == 5 && (atoi(argv[i]) > 10000 || atoi(argv[i]) < 0)) {
            fprintf(stderr, "The 5 argument must be >=0 and <=10000\n");
            return 1;
        }
    }


    int NZ = atoi(argv[1]);//number of customers
    int NU = atoi(argv[2]); //number of clerks
    int TZ = atoi(argv[3]); //The maximum time in milliseconds that a customer waits after their creation before entering the post office.
    int TU = atoi(argv[4]); //Maximum length of a clerk's break in milliseconds.
    int F = atoi(argv[5]);  //Maximum time in milliseconds after which the mail is closed for new arrivals.

    //current processor
    process_t curr_process;
    //main processor
    process_t main_process = create_process(getpid(), 0, 'm');

    //creating shared memory pointer
    ipc_t *ipc = ipc_init();
    if (ipc == NULL) {
        // IPC initialization failed, handling the error
        fprintf(stderr, "IPC initialization failed");
        exit(EXIT_FAILURE);
    }
    //open the file or create new if not existed
    FILE* file = fopen(FILENAME, "w");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file");
        exit(EXIT_FAILURE);
    }

    // unlink semaphores if existed before
    unlink_semaphores();
    // INITIALIZE SEMAPHORES

    //initialize 3 semaphores for queue (3 services 3 queue)
    sem_t *sem_queue[3];
    for (int i = 0; i < 3; i++) {
        char name[50];
        sprintf(name, "%s%d", SEMAPHORE_QUEUE, i);
        //unlink the semaphore if existed before
        sem_queue[i] = sem_open(name, O_CREAT, 0660, 0);
        if(sem_queue[i] == SEM_FAILED){
            fprintf(stderr, "Failed to initialize queue semaphores\n");
            exit(EXIT_FAILURE);
        }
    }

    //initialize mutex for post
    sem_t *mutex_post = sem_open(MUTEX_POST, O_CREAT, 0660, 1);
    if(mutex_post == SEM_FAILED){
        fprintf(stderr, "Failed to initialize post mutex\n");
        exit(EXIT_FAILURE);
    }

    //initialize semaphore(mutex) for writing in file
    sem_t *sem_file = sem_open(SEMAPHORE_WFILE, O_CREAT, 0660, 1);
    if(sem_file == SEM_FAILED){
        fprintf(stderr, "Failed to initialize file semaphore\n");
        exit(EXIT_FAILURE);
    }
    //initialize mutex for sync editing data about queue
    sem_t *mutex_queue = sem_open(MUTEX_QUEUE, O_CREAT, 0660, 1);
    if(mutex_queue == SEM_FAILED){
        fprintf(stderr, "Failed to initialize queue mutex\n");
        exit(EXIT_FAILURE);
    }

    // array of pointers to all semaphores for easier cleaning up
    sem_t *sem_array[SEM_COUNT] = { sem_file, mutex_queue, sem_queue[0], sem_queue[1], sem_queue[2], mutex_post};

    //Creates NU processes out of main process
    if (main_process.pid == getpid())
    {
        for (int i = 1; i <= NU; i++) {
            if (fork() == 0) {
                curr_process = create_process(getpid(), ++ipc->clerks_n,'U');
                break;
            }
        }
    }
    //Creates NZ processes out of main process
    if (main_process.pid == getpid())
    {
        for (int i = 1; i <= NZ; i++) {
            if (fork() == 0) {
                curr_process = create_process(getpid(), ++ipc->customer_n, 'Z');
                break;
            }
        }
    }

    // initialize random number generator
    srand(getpid()); 

    if(main_process.pid == getpid()) {
        // main process

        // defines the work time
        if (F != 0) {
            int sleep_time = ((rand() % (F / 2 + 1)) + (F / 2)) * 1000; // random time in ms
            usleep(sleep_time);
        }

        //waiting a semaphore for post
        sem_wait(mutex_post);
        ipc->status_post = false;
        //waiting for the semaphore to be free then write in a file
        sem_wait(sem_file);
        print_msg(file, "%u: closing\n", ++ipc->line_n);
        sem_post(sem_file);
        //posting the post semaphore for other usages
        sem_post(mutex_post);

    }
    else{
        if(curr_process.type == 'Z') {
            //waiting for the semaphore to be free then write in a file
            sem_wait(sem_file);
            print_msg(file, "%u: Z %d: started\n", ++ipc->line_n, curr_process.id);
            sem_post(sem_file);
            srand(getpid()); // initialize random number generator
            if (TZ != 0) {
                int sleep_time = (rand() % (TZ + 1)) * 1000; // random sleep in ms
                usleep(sleep_time);
            }
            //waiting a semaphore for post( no entering office after closing)
            sem_wait(mutex_post);
            if (ipc->status_post == true) {

                int service = rand() % 3 + 1;

                //waiting for the semaphore to be free then write in a file
                sem_wait(sem_file);
                print_msg(file, "%u: Z %u: entering office for a service %d\n", ++ipc->line_n,
                          curr_process.id, service);
                sem_post(sem_file);
                sem_post(mutex_post);
                //semaphore for correct accessing queue data
                sem_wait(mutex_queue);
                ipc->queue[service - 1]++;
                sem_post(mutex_queue);

                //waiting for free clerk to start
                sem_wait(sem_queue[service - 1]);

                //waiting for the semaphore to be free then write in a file
                sem_wait(sem_file);
                print_msg(file, "%u: Z %d: called by office worker\n", ++ipc->line_n, curr_process.id);
                sem_post(sem_file);

                //wait random amount of time 0-10
                usleep((rand() % 11) * 1000);
            } else{
                //if the post is closed - post the mutex and go home
                sem_post(mutex_post);
            }
            //waiting for the semaphore to be free then write in a file
            sem_wait(sem_file);
            print_msg(file, "%u: Z %d: going home\n", ++ipc->line_n, curr_process.id);
            sem_post(sem_file);

            // Destroy the semaphores
            destroy_semaphores(sem_array);
            //close file
            fclose(file);
            //exit with success
            exit(EXIT_SUCCESS);

        }else if (curr_process.type == 'U'){
            //waiting for the semaphore to be free then write in a file
            sem_wait(sem_file);
            print_msg(file, "%u: U %d: started\n", ++ipc->line_n, curr_process.id);
            sem_post(sem_file);
            while(!no_queue(mutex_queue, ipc) || ipc->status_post == true) {
                int service = rand() % 3 + 1;
                //choose random service to serve if no found - take a break
                while (ipc->queue[service - 1] == 0) {
                    service = rand() % 3 + 1;
                    // is closed and no customers - going home
                    //waiting a semaphore for post
                    sem_wait(mutex_post);
                    if(no_queue(mutex_queue, ipc) && ipc->status_post == false){
                        sem_post(mutex_post);
                        //waiting for the semaphore to be free then write in a file
                        sem_wait(sem_file);
                        print_msg(file, "%u: U %d: going home\n", ++ipc->line_n, curr_process.id);
                        sem_post(sem_file);

                        // Destroy the semaphores
                        destroy_semaphores(sem_array);
                        //close file
                        fclose(file);
                        //exit with success
                        exit(EXIT_SUCCESS);
                    }
                    else{
                        sem_post(mutex_post);
                    }
                    //taking break
                    //waiting a semaphore for post(no taking break after closing)
                    sem_wait(mutex_post);
                    if (no_queue(mutex_queue,ipc) && ipc->status_post == true) {
                        //waiting for the semaphore to be free then write in a file
                        sem_wait(sem_file);
                        print_msg(file, "%u: U %d: taking break\n", ++ipc->line_n, curr_process.id);
                        sem_post(sem_file);
                        sem_post(mutex_post);
                        if(TU != 0){
                            usleep((rand() % (TU + 1)) * 1000);
                        }
                        //waiting for the semaphore to be free then write in a file
                        sem_wait(sem_file);
                        print_msg(file, "%u: U %d: break finished\n", ++ipc->line_n, curr_process.id);
                        sem_post(sem_file);
                    }
                    else{
                        sem_post(mutex_post);
                    }
                }

                // check if the service was already taken by someone
                sem_wait(mutex_queue);
                if(ipc->queue[service - 1] != 0){
                    ipc->queue[service - 1]--;
                    sem_post(mutex_queue);

                }
                else{
                    // the queue was already taken by another customer
                    sem_post(mutex_queue);
                    continue;
                }

                //ready for the next customer
                sem_post(sem_queue[service - 1]);

                //waiting for the semaphore to be free then write in a file
                sem_wait(sem_file);
                print_msg(file, "%u: U %d: serving a service of type %d\n", ++ipc->line_n, curr_process.id,
                          service);
                sem_post(sem_file);

                usleep((rand() % 11) * 1000);
                //waiting for the semaphore to be free then write in a file
                sem_wait(sem_file);
                print_msg(file, "%u: U %d: service finished\n", ++ipc->line_n, curr_process.id);
                sem_post(sem_file);

            }
            //going home

            //waiting for the semaphore to be free then write in a file
            sem_wait(sem_file);
            print_msg(file, "%u: U %d: going home\n", ++ipc->line_n, curr_process.id);
            sem_post(sem_file);
            // Destroy the semaphores
            destroy_semaphores(sem_array);
            //close file
            fclose(file);
            //exit with success
            exit(EXIT_SUCCESS);
        }

    }

    while(wait(NULL) > 0) ;


    // Destroy the semaphores
    destroy_semaphores(sem_array);

    // Unlink the semaphores
    unlink_semaphores();

    // Close file
    fclose(file);

    //Destroy shared memory
    ipc_destroy(ipc);

    return 0;
}