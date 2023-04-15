#include "header.h"



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
                fprintf(stderr, "The arguments must be numbers\n");
                return 1;
            }
        }
//        if(atoi(argv[i]) < 0){
//            fprintf(stderr, "The arguments must be unsigned");
//        }
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



    // unlink semaphores if existed before
    sem_unlink(SEMAPHORE_CUSTOMER);
    sem_unlink(SEMAPHORE_CLERC);
    sem_unlink(SEMAPHORE_WFILE);

    FILE * file;

    file = fopen(FILENAME, "w");
    // INITIALIZE SEMAPHORES
    sem_t *sem_customer = sem_open(SEMAPHORE_CUSTOMER, O_CREAT, 0660, 0);
    if(sem_customer == SEM_FAILED){
        perror("sem_open/customer");
        exit(EXIT_FAILURE);
    }
    sem_t *sem_clerk = sem_open(SEMAPHORE_CLERC, O_CREAT, 0660, NU);
    if(sem_clerk == SEM_FAILED){
        perror("sem_open/clerc");
        exit(EXIT_FAILURE);
    }

    sem_t *sem_file = sem_open(SEMAPHORE_WFILE, O_CREAT, 0660, 1);
    if(sem_file == SEM_FAILED){
        perror("sem_open/wfile");
        exit(EXIT_FAILURE);
    }
    /*
     *   Creates NZ processes out of main process
     */
    if (main_process.pid == getpid())
    {
        for (int i = 1; i <= NZ; i++) {
            if (fork() == 0) {
                curr_process = create_process(getpid(), ++ipc->customer_n, 'Z');
                print_msg(file, sem_file, "%u: Z %d: started\n", ++ipc->line_n, curr_process.id);
                srand(getpid()); // initialize random number generator
                int sleep_time = (rand() % TZ) * 1000; // random sleep in ms
                usleep(sleep_time);
                break;
            }
        }
    }

    /*
     *   Creates NU processes out of main process
     */
    if (main_process.pid == getpid())
    {
        for (int i = 1; i <= NU; i++) {
            if (fork() == 0) {
                curr_process = create_process(getpid(), ++ipc->clerks_n,'U');
                print_msg(file,  sem_file, "%u: U %d: started\n", ++ipc->line_n, curr_process.id);
                break;
            }
        }
    }


    srand(getpid()); // initialize random number generator

    if(main_process.pid != getpid()){
        if(curr_process.type == 'Z') {
            if(ipc->is_post_opened == true) {
                int service = rand() % 3 + 1;
                print_msg(file, sem_file, "%u: Z %u: entering office for a service %d\n", ++ipc->line_n, curr_process.id, service);
//                while(ipc->queue[service - 1] != 0){
//                    if(ipc->is_post_opened == false){
//                        printf("Z %d: going home\n", curr_process.id);
//                        exit(EXIT_SUCCESS);
//                    }
//                }
                ipc->queue[service - 1]++;


                    //sem_wait(sem_clerk);

                    sem_wait(sem_clerk);
                if(ipc->is_post_opened == true) {
                    print_msg(file, sem_file, "%u: Z %d: called by office worker\n", ++ipc->line_n, curr_process.id);
                    sem_post(sem_customer);
                    usleep((rand() % (10 + 1)) * 1000);
                    print_msg(file, sem_file, "%u: Z %d: going home\n", ++ipc->line_n, curr_process.id);
                    exit(EXIT_SUCCESS);
                }
            }
            else {
                print_msg(file, sem_file,"%u: Z %d: going home\n", ++ipc->line_n, curr_process.id);
                exit(EXIT_SUCCESS);
            }
        }else if (curr_process.type == 'U'){
            while(ipc->is_post_opened == true) {
                int service = rand() % 3 + 1;

                while (ipc->queue[service - 1] == 0 && ipc->is_post_opened == true) {
                    if (ipc->queue[0] == 0 && ipc->queue[1] == 0 && ipc->queue[2] == 0) {
                        sem_wait(sem_clerk);

                        print_msg(file, sem_file, "%u: U %d: taking break\n", ++ipc->line_n, curr_process.id);
                        usleep((rand() % TU) * 1000);
                        print_msg(file, sem_file, "%u: U %d: break finished\n", ++ipc->line_n, curr_process.id);
                        sem_post(sem_clerk);

                        continue;
                    }
                    service = rand() % 3 + 1;
                }

                if (ipc->is_post_opened == true) {
                    ipc->queue[service - 1]--;
                    sem_wait(sem_customer);
                    print_msg(file, sem_file, "%u: U %d: serving a service of type %d\n", ++ipc->line_n, curr_process.id, service);
                    usleep((rand() % (10 + 1)) * 1000);
                    print_msg(file, sem_file, "%u: U %d: service finished\n", ++ipc->line_n, curr_process.id);
                    sem_post(sem_clerk);
                }

            }
            print_msg(file,sem_file, "%u: U %d: going home\n", ++ipc->line_n, curr_process.id);
            exit(EXIT_SUCCESS);
        }

    }
    else{
        int sleep_time = ((rand() % (F/2 + 1)) + F/2) * 1000; // random time in ms
        usleep(sleep_time);
        ipc->is_post_opened = false;
        print_msg(file, sem_file, "%u: closing\n", ++ipc->line_n);
    }
    while(wait(NULL) > 0) ;
    // Destroy the semaphores
    sem_close(sem_clerk);
    sem_close(sem_customer);
    sem_close(sem_file);

    //Destroy shared memory
    fclose(file);
    ipc_destroy(ipc);
    return 0;
}