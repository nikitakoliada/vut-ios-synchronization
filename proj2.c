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
                fprintf(stderr, "The arguments must be numbers\n");
                return 1;
            }
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

    //open the file or create new if not existed
    FILE* file = fopen(FILENAME, "w");
    if (file == NULL) {
        perror("fopen");
        return 1;
    }

    // unlink semaphores if existed before
    sem_unlink(SEMAPHORE_CUSTOMER);
    sem_unlink(SEMAPHORE_WFILE);
    sem_unlink(SEMAPHORE_QUEUE);



    // INITIALIZE SEMAPHORES

    //initialize 3 semaphores for customer queue (3 services 3 queue)
    sem_t *sem_customer[3];
    for (int i = 0; i < 3; i++) {
        char name[50];
        sprintf(name, "%s%d", SEMAPHORE_CUSTOMER, i);
        //unlink the semaphore if existed before
        sem_unlink(name);
        sem_customer[i] = sem_open(name, O_CREAT, 0660, 0);
        if(sem_customer[i] == SEM_FAILED){
            perror("sem_open/customer");
            exit(EXIT_FAILURE);
        }
    }

    //initialize 3 semaphores for clerk queue (3 services 3 queue)
    sem_t *sem_clerk[3];
    for (int i = 0; i < 3; i++) {
        char name[50];
        sprintf(name, "%s%d", SEMAPHORE_CLERC, i);
        //unlink the semaphore if existed before
        sem_unlink(name);
        sem_clerk[i] = sem_open(name, O_CREAT, 0660, 0);
        if(sem_clerk[i] == SEM_FAILED){
            perror("sem_open/clerk");
            exit(EXIT_FAILURE);
        }
    }

    //initialize semaphore(mutex) for writing in file
    sem_t *sem_file = sem_open(SEMAPHORE_WFILE, O_CREAT, 0660, 1);
    if(sem_file == SEM_FAILED){
        perror("sem_open/wfile");
        exit(EXIT_FAILURE);
    }
    //initialize semaphore(mutex) for data about queue
    sem_t *sem_queue = sem_open(SEMAPHORE_QUEUE, O_CREAT, 0660, 1);
    if(sem_file == SEM_FAILED){
        perror("sem_open/queue");
        exit(EXIT_FAILURE);
    }

    // array of pointers to all semaphores for easier cleaning up
    sem_t *sem_array[SEM_COUNT] = { sem_file, sem_queue, sem_customer[0], sem_customer[1], sem_customer[2], sem_clerk[0], sem_clerk[1], sem_clerk[2]};

    /*
 *   Creates NU processes out of main process
 */
    if (main_process.pid == getpid())
    {
        for (int i = 1; i <= NU; i++) {
            if (fork() == 0) {
                curr_process = create_process(getpid(), ++ipc->clerks_n,'U');
                print_msg(file,sem_file, "%u: U %d: started\n", ++ipc->line_n, curr_process.id);
                break;
            }
        }
    }
    /*
     *   Creates NZ processes out of main process
     */
    if (main_process.pid == getpid())
    {
        for (int i = 1; i <= NZ; i++) {
            if (fork() == 0) {
                curr_process = create_process(getpid(), ++ipc->customer_n, 'Z');
                print_msg( file,sem_file, "%u: Z %d: started\n", ++ipc->line_n, curr_process.id);
                srand(getpid()); // initialize random number generator
                int sleep_time = (rand() % TZ) * 1000; // random sleep in ms
                usleep(sleep_time);
                break;
            }
        }
    }


    srand(getpid()); // initialize random number generator

    if(main_process.pid != getpid()){
        if(curr_process.type == 'Z') {
            if(ipc->is_post_opened == true) {
                int service = rand() % 3 + 1;
                print_msg( file,sem_file, "%u: Z %u: entering office for a service %d\n", ++ipc->line_n,
                          curr_process.id, service);

                //semaphore for correct accessing queue data
                sem_wait(sem_queue);
                ipc->queue[service - 1]++;
                sem_post(sem_queue);

                //waiting for free clerk to start
                sem_wait(sem_clerk[service - 1]);
                print_msg(file,sem_file, "%u: Z %d: called by office worker\n", ++ipc->line_n, curr_process.id);

                //semaphore for notifying clerk to start serving the customer with specific service
                sem_post(sem_customer[service - 1]);

                usleep((rand() % 10) * 1000);
                print_msg(file,sem_file, "%u: Z %d: going home\n", ++ipc->line_n, curr_process.id);
                //destroying semaphores
                destroy_semaphores(sem_array);
                //closing
                fclose(file);
                exit(EXIT_SUCCESS);

            }
            else {
                print_msg(file,sem_file,"%u: Z %d: going home\n", ++ipc->line_n, curr_process.id);
                destroy_semaphores(sem_array);
                fclose(file);
                exit(EXIT_SUCCESS);
            }
        }else if (curr_process.type == 'U'){
            while(ipc->is_post_opened == true || !no_queue(ipc)) {
                int service = rand() % 3 + 1;
                //choose random service to serve if no found - take a break
                while (ipc->queue[service - 1] == 0) {
                    service = rand() % 3 + 1;
                    if(!ipc->is_post_opened && no_queue(ipc)){
                        // is closed and no customers - going home
                        print_msg(file,sem_file, "%u: U %d: going home\n", ++ipc->line_n, curr_process.id);
                        destroy_semaphores(sem_array);
                        fclose(file);
                        exit(EXIT_SUCCESS);
                    }
                    if (no_queue(ipc)) {
                        //taking break
                        print_msg(file,sem_file, "%u: U %d: taking break\n", ++ipc->line_n, curr_process.id);
                        usleep((rand() % TU) * 1000);
                        print_msg(file,sem_file, "%u: U %d: break finished\n", ++ipc->line_n, curr_process.id);
                    }
                }

                //ready for the next customer
                sem_post(sem_clerk[service - 1]);

                //semaphore for correct accessing queue data
                sem_wait(sem_queue);
                ipc->queue[service - 1]--;
                sem_post(sem_queue);


                //if the customer is ready - serve
                sem_wait(sem_customer[service - 1]);

                print_msg(file,sem_file, "%u: U %d: serving a service of type %d\n", ++ipc->line_n, curr_process.id,
                          service);
                usleep((rand() % 10) * 1000);
                print_msg(file,sem_file, "%u: U %d: service finished\n", ++ipc->line_n, curr_process.id);

            }
            print_msg(file, sem_file, "%u: U %d: going home\n", ++ipc->line_n, curr_process.id);
            destroy_semaphores(sem_array);
            fclose(file);
            exit(EXIT_SUCCESS);
        }

    }
    else{
        // main process
        // defines the work time
        int sleep_time = ((rand() % (F/2 + 1)) + F/2) * 1000; // random time in ms
        usleep(sleep_time);
        print_msg(file, sem_file, "%u: closing\n", ++ipc->line_n);
        ipc->is_post_opened = false;

    }
    while(wait(NULL) > 0) ;


    // Destroy the semaphores
    destroy_semaphores(sem_array);


    //unlink the semaphores
//    sem_unlink(SEMAPHORE_CUSTOMER);
//    sem_unlink(SEMAPHORE_CLERC);
//    sem_unlink(SEMAPHORE_WFILE);
//    sem_unlink(SEMAPHORE_QUEUE);

    //close file
    fclose(file);

    //Destroy shared memory
    ipc_destroy(ipc);
    return 0;
}