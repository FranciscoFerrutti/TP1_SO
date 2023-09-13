#include "pipe_manager.h"
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define CHILD_QTY 1
#define STARTING_FILE 1
#define INITIAL_FILES_PER_CHILD 2
#define SHARED_MEMORY_SIZE 1024 // Adjust this size as needed
#define SHARED_MEMORY_NAME "/my_shared_memory"

int first_files_per_child(int argc, const char *argv[], int parent_to_child_pipe[][2], int child_to_parent_pipe[][2]);


int main(int argc, const char *argv[]) {

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

    fd_set readfds;
    FD_ZERO(&readfds);
    int max_fd = -1;

    for (int i = 0; i < CHILD_QTY; i++) {
        if (child_to_parent_pipe[i][0] > max_fd) {
            max_fd = child_to_parent_pipe[i][0];
        }
        FD_SET(child_to_parent_pipe[i][0], &readfds); // Add slave pipes to the set
    }

    if (max_fd < 0) {
        perror("file descriptor");
        exit(EXIT_FAILURE);
    }

    int current_file = 1;
    int files_assigned = first_files_per_child(argc, argv, parent_to_child_pipe, child_to_parent_pipe);

    while (current_file < argc || files_assigned < argc) {
        int ready = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (ready > 0) {
            for (int i = 0; i < CHILD_QTY; i++) {
                if (FD_ISSET(child_to_parent_pipe[i][0], &readfds)) {
                    pipe_read(child_to_parent_pipe[i][0], child_md5[i]);
                    printf("pipe content from child %d: %s\n", i, child_md5[i]);

                    // Write data to shared memory with semaphore
                    sem_wait(app_semaphore);
                    strcpy(shared_memory, child_md5[i]);
                    sem_post(view_semaphore);

                    if (files_assigned < argc) {
                        write(parent_to_child_pipe[i][1], argv[files_assigned], strlen(argv[files_assigned]) + 1);
                        files_assigned++;
                    } else {
                        // No more files to assign, signal to exit
                        write(parent_to_child_pipe[i][1], "", 1);
                    }
                }
            }
        }
    }

    for (int i = 0; i < CHILD_QTY; i++) {
        close(parent_to_child_pipe[i][1]);
        close(child_to_parent_pipe[i][0]);
        waitpid(child_pid[i], NULL, 0);
    }

    // Close and unlink semaphores
    sem_close(app_semaphore);
    sem_close(view_semaphore);
    sem_unlink("/app_semaphore");
    sem_unlink("/view_semaphore");

    close(shm_fd);
    shm_unlink(SHARED_MEMORY_NAME);
    return EXIT_SUCCESS;
}

int first_files_per_child(int argc, const char *argv[], int parent_to_child_pipe[][2], int child_to_parent_pipe[][2]) {
    int i;
    int files_assigned = 0;

    for (i = STARTING_FILE; i < argc && i <= INITIAL_FILES_PER_CHILD * CHILD_QTY; i++) {
        pipe_write(parent_to_child_pipe[i % CHILD_QTY][TO_WRITE], argv[i]);
        files_assigned++;
    }

    return files_assigned;
}
