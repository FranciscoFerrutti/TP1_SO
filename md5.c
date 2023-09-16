//md5.c
#include "pipe_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>

// Constants
#define CHILD_QTY 5
#define INITIAL_FILES_PER_CHILD 2
#define SHARED_MEMORY_SIZE 100000
#define SHARED_MEMORY_NAME "/my_shared_memory"
#define SHM_SEMAPHORE_NAME "/shm_semaphore"
#define AVAIL_SEMAPHORE_NAME "/avail_semaphore"
#define INFO_TEXT "PID:%d - %s"

// Function to distribute files to slaves
int distribute_files(int argc, const char *argv[], int parent_to_child_pipe[][2], int child_to_parent_pipe[][2]);
void print_in_shm(int idx, int pid, char * path, char * md5);

int main(int argc, const char *argv[]) {
    int vision_opened=0;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
     
        // Open shared memory and semaphores
    sleep(2);

    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    char *shared_memory = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    

    sem_t *shm_semaphore = sem_open(SHM_SEMAPHORE_NAME, 1); // Open existing semaphore
    if (shm_semaphore == SEM_FAILED) {
        perror("sem_open (shm_semaphore)");
        printf("No view detected\n");
    } else {
        vision_opened++;
    }

    
    sem_t *avail_semaphore = sem_open(AVAIL_SEMAPHORE_NAME, 0); // Open existing semaphore
    if (avail_semaphore == SEM_FAILED) {
        perror("sem_open (avail_semaphore)");
        printf("No view detected\n");
    } else {
        vision_opened++;
    }

    if(vision_opened==1){
        perror("missing 1 semaphore");
        exit(EXIT_FAILURE);
    }

    char child_md5[CHILD_QTY][MAX_MD5 + MAX_PATH + 4];
    pid_t child_pid[CHILD_QTY];
    int parent_to_child_pipe[CHILD_QTY][2];
    int child_to_parent_pipe[CHILD_QTY][2];

    if (ftruncate(shm_fd, SHARED_MEMORY_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    FILE *resultado_file = fopen("resultado.txt", "wr");
    if (resultado_file == NULL) {
        perror("fopen(resultado.txt)");
        exit(EXIT_FAILURE);
    }



    // Create pipes and fork child processes
    for (int i = 0; i < CHILD_QTY; i++) {
        if (pipe(parent_to_child_pipe[i]) == -1 || pipe(child_to_parent_pipe[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        child_pid[i] = fork();

        if (child_pid[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (child_pid[i] == 0) {
            close(parent_to_child_pipe[i][1]);
            close(child_to_parent_pipe[i][0]);

            // Redirect stdin and stdout
            dup2(parent_to_child_pipe[i][0], STDIN_FILENO);
            close(parent_to_child_pipe[i][0]);

            dup2(child_to_parent_pipe[i][1], STDOUT_FILENO);
            close(child_to_parent_pipe[i][1]);

            char *args[] = {"./slave.elf", NULL};
            execve(args[0], args, NULL);
            perror("execve");
            exit(EXIT_FAILURE);
        }
    }

    // Initialize file distribution
   

    // Main processing loop
    fd_set readfds;
    FD_ZERO(&readfds);
    int max_fd = -1;

    for (int i = 0; i < CHILD_QTY; i++) {
        if (child_to_parent_pipe[i][0] > max_fd) {
            max_fd = child_to_parent_pipe[i][0];
        }
        FD_SET(child_to_parent_pipe[i][0], &readfds); // Add slave pipes to the set
    }
    int files_assigned = distribute_files(argc, argv, parent_to_child_pipe, child_to_parent_pipe);
    int current_file_index = 0;
    int info_length=strlen(INFO_TEXT)+MAX_MD5+MAX_PATH+2;

   while (current_file_index < argc) {
    
        fd_set readfds;
        FD_ZERO(&readfds);
        int max_fd = -1;
       
        for (int i = 0; i < CHILD_QTY; i++) {
            if (child_to_parent_pipe[i][0] > max_fd) {
                max_fd = child_to_parent_pipe[i][0];
            }
            FD_SET(child_to_parent_pipe[i][0], &readfds); // Add slave pipes to the set
        }

        int ready = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        if(vision_opened==2){
            sem_wait(shm_semaphore);
        }
        for (int i = 0; i < CHILD_QTY; i++) {
            if (FD_ISSET(child_to_parent_pipe[i][0], &readfds)) {
                // Read MD5 hash from the child
                int bytes_read = pipe_read(child_to_parent_pipe[i][0], child_md5[i]);
                if (bytes_read < 0) {
                    perror("read");
                    exit(EXIT_FAILURE);
                } 
                else if (bytes_read == 0) {
                    // No more data from this child, remove it from the set
                    close(child_to_parent_pipe[i][0]);
                    FD_CLR(child_to_parent_pipe[i][0], &readfds);
                } 
                else {
                    // printf("PID:%d Received MD5 hash of file %s from child %d: %s\n", child_pid[i], argv[current_file_index], i, child_md5[i]);
                     fprintf(resultado_file, INFO_TEXT "\n", child_pid[i], child_md5[i]);
                     fflush(resultado_file); // Flush the file buffer
                    sprintf(shared_memory+current_file_index*info_length, INFO_TEXT, child_pid[i], child_md5[i]);
                  
                   // printf("%d - %s \n", child_pid[i], child_md5[i]);

                    //printf("%d files_assigned  %d current_file_index   %d\n", files_assigned, current_file_index, argc);
                    if(files_assigned < argc) {
                        pipe_write(parent_to_child_pipe[i][1], argv[files_assigned++]);  
                    }
                    else{
                        close(parent_to_child_pipe[i][1]);
                    }
                    //strcpy(shared_memory, child_md5[i]);
                    
                    current_file_index++; // Move to the next file
                
                }
            }
        }
        sprintf(shared_memory+current_file_index*info_length, "\n");
        if(vision_opened==2){
            sem_post(shm_semaphore);
            sem_post(avail_semaphore);
        }
    }
    sprintf(shared_memory+current_file_index*info_length, "\t");



    // Wait for child processes to finish
    for (int i = 0; i < CHILD_QTY; i++) {
        waitpid(child_pid[i], NULL, 0);
        close(parent_to_child_pipe[i][1]);
    }

    // Close and unlink semaphores
    sem_close(shm_semaphore);
    sem_unlink(SHM_SEMAPHORE_NAME);

    sem_close(avail_semaphore);
    sem_unlink(AVAIL_SEMAPHORE_NAME);

    // Close and unlink shared memory
    close(shm_fd);
    shm_unlink(SHARED_MEMORY_NAME);
    return EXIT_SUCCESS;
}

int distribute_files(int argc, const char *argv[], int parent_to_child_pipe[][2], int child_to_parent_pipe[][2]) {
    int files_assigned = 0;
    for (int child_index = 0; child_index < CHILD_QTY; child_index++) {
        for (int i=0; i < INITIAL_FILES_PER_CHILD ; i ++) {
            // Distribute files to child processes in a round-robin manner
            pipe_write(parent_to_child_pipe[child_index][1], argv[files_assigned++]);  
        }
    }
    return files_assigned;
}

void print_in_shm(int idx, int pid, char * path, char * md5){
        
}
