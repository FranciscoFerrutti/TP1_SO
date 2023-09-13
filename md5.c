
//md5.c
#include "pipe_manager.h"
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h> 
#include <sys/shm.h>
#include <sys/mman.h>
#define CHILD_QTY 1
#define STARTING_FILE 1
#define INITIAL_FILES_PER_CHILD 2
#define SHARED_MEMORY_SIZE 1024 // Same size as in md5.c
#define SHARED_MEMORY_NAME "/my_shared_memory" 
int main(int argc, const char * argv[]){
    
     if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Declare and initialize semaphores
    sem_t *app_semaphore = sem_open("/app_semaphore", O_CREAT, 0666, 1);
    if (app_semaphore == SEM_FAILED) {
        perror("sem_open (app_semaphore)");
        exit(EXIT_FAILURE);
    }

    sem_t *view_semaphore = sem_open("/view_semaphore", O_CREAT, 0666, 0);
    if (view_semaphore == SEM_FAILED) {
        perror("sem_open (view_semaphore)");
        exit(EXIT_FAILURE);
    }

    char child_md5[CHILD_QTY][MAX_MD5 + 1];
    pid_t child_pid[CHILD_QTY];
    int parent_to_child_pipe[CHILD_QTY][2];
    int child_to_parent_pipe[CHILD_QTY][2];

    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, SHARED_MEMORY_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    char *shared_memory = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }


    
    for (int i = 0; i < CHILD_QTY; i++) {
        child_pid[i] = fork();

        if (child_pid[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (child_pid[i] == 0) {

            close(parent_to_child_pipe[i][1]);
            close(child_to_parent_pipe[i][0]);

            //code goes here
            close(STDIN_FILENO);
            dup(parent_to_child_pipe[i][0]);
            close(parent_to_child_pipe[i][0]);

            close(STDOUT_FILENO);
            dup(child_to_parent_pipe[i][1]);
            close(child_to_parent_pipe[i][1]);

            char *args[] = {"./slave.elf", NULL}; 
            execve("./slave.elf", args, NULL);
            perror("execve");
            exit(EXIT_FAILURE);



        }
        
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    int max_fd=-1;

    for (int i = 0; i < CHILD_QTY; i++) {
        if(child_to_parent_pipe[i][0]>max_fd){
            max_fd = child_to_parent_pipe[i][0];
        }
        FD_SET(child_to_parent_pipe[i][0], &readfds); // Add slave pipes to the set
    }

    if(max_fd<0){
        perror("file descriptor");
        exit(EXIT_FAILURE);
    }

    int current_file = 1;
    while (current_file < argc) {
        int ready = select(max_fd, &readfds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (ready > 0) {
            for (int i = 0; i < CHILD_QTY && current_file < argc; i++) {
                if (FD_ISSET(child_to_parent_pipe[i][0], &readfds)) {
                    pipe_read(child_to_parent_pipe[i][0], child_md5[i]);
                    printf("pipe content from child %d: %s\n", i, child_md5[i]);

                    // Write data to shared memory with semaphore
                    sem_wait(app_semaphore);
                    strcpy(shared_memory, child_md5[i]);
                    sem_post(view_semaphore);

                    write(parent_to_child_pipe[i][1], argv[current_file], strlen(argv[current_file]) + 1);
                    current_file++;
                }
            }
        }
    }

    for (int i = 0; i < CHILD_QTY; i++) {
        close(parent_to_child_pipe[i][1]);
        close(child_to_parent_pipe[i][0]);
        waitpid(child_pid[i], NULL, 0);
    }
    close(shm_fd);
    shm_unlink(SHARED_MEMORY_NAME);
    return EXIT_SUCCESS;
}

int first_files_per_child(int argc, const char * argv[], int ** parent_to_child_pipe, int ** child_to_parent_pipe){
    int i;

    for(i=STARTING_FILE; i<argc && i<=INITIAL_FILES_PER_CHILD*CHILD_QTY; i++){
        pipe_write(parent_to_child_pipe[i%CHILD_QTY][1], argv[i]);
    }

    return i;
}
