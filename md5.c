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
#define MAX_MD5 32
#define SHARED_MEMORY_SIZE 1024
#define SHARED_MEMORY_NAME "/my_shared_memory"
#define APP_SEMAPHORE_NAME "/app_semaphore"
#define VIEW_SEMAPHORE_NAME "/view_semaphore"

// Function to distribute files to slaves
int distribute_files(int argc, const char *argv[], int parent_to_child_pipe[][2], int child_to_parent_pipe[][2]);

int main(int argc, const char *argv[]) {
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Declare and initialize semaphores
    sem_t *app_semaphore = sem_open(APP_SEMAPHORE_NAME, O_CREAT, 0666, 1);
    if (app_semaphore == SEM_FAILED) {
        perror("sem_open (app_semaphore)");
        exit(EXIT_FAILURE);
    }

    sem_t *view_semaphore = sem_open(VIEW_SEMAPHORE_NAME, O_CREAT, 0666, 0);
    if (view_semaphore == SEM_FAILED) {
        perror("sem_open (view_semaphore)");
        exit(EXIT_FAILURE);
    }

    char child_md5[CHILD_QTY][MAX_MD5 + 1];
    pid_t child_pid[CHILD_QTY];
    int parent_to_child_pipe[CHILD_QTY][2];
    int child_to_parent_pipe[CHILD_QTY][2];

    // Create shared memory
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
    int files_assigned = distribute_files(argc, argv, parent_to_child_pipe, child_to_parent_pipe);

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

    while (1) {
        int ready = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (ready > 0) {
            for (int i = 0; i < CHILD_QTY; i++) {
                if (FD_ISSET(child_to_parent_pipe[i][0], &readfds)) {
                    // Read MD5 hash from the child
                    int bytes_read = pipe_read(child_to_parent_pipe[i][0], child_md5[i]);
                    if (bytes_read < 0) {
                        perror("read");
                        exit(EXIT_FAILURE);
                    } else if (bytes_read == 0) {
                        // No more data from this child, remove it from the set
                        close(child_to_parent_pipe[i][0]);
                        FD_CLR(child_to_parent_pipe[i][0], &readfds);
                    } else {
                        // child_md5[i][bytes_read] = '\0';
                        printf("Received MD5 hash from child %d: %s\n", i, child_md5[i]);
                        

                        // Write data to shared memory with semaphore
                        strcpy(shared_memory, child_md5[i]);
                        sem_post(view_semaphore);

                        if (files_assigned < argc) {
                            pipe_write(parent_to_child_pipe[i][1], argv[files_assigned]);
                            files_assigned++;
                        } else {
                            // No more files to assign, signal to exit
                            close(parent_to_child_pipe[i][1]);
                        }
                    }
                }
            }
        }
    }
    // Signal view process that all files are processed
    sem_post(view_semaphore);

    // Wait for child processes to finish
    for (int i = 0; i < CHILD_QTY; i++) {
        waitpid(child_pid[i], NULL, 0);
        close(parent_to_child_pipe[i][1]);
    }

    // Close and unlink semaphores
    sem_close(app_semaphore);
    sem_close(view_semaphore);
    sem_unlink(APP_SEMAPHORE_NAME);
    sem_unlink(VIEW_SEMAPHORE_NAME);

    // Close and unlink shared memory
    close(shm_fd);
    shm_unlink(SHARED_MEMORY_NAME);
    return EXIT_SUCCESS;
}

int distribute_files(int argc, const char *argv[], int parent_to_child_pipe[][2], int child_to_parent_pipe[][2]) {
    int files_assigned = 0;

    for (int i = 1; i < argc && files_assigned < (CHILD_QTY * INITIAL_FILES_PER_CHILD); i++) {
        write(parent_to_child_pipe[files_assigned % CHILD_QTY][1], argv[i], strlen(argv[i]) + 1);
        files_assigned++;
    }

    return files_assigned;
}
