// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
//#define INFO_TEXT "PID:%d - %s"
#define INFO_TEXT "PID:%d - %s"

void initialize_resources(int *shm_fd, char **shared_memory, sem_t **shm_semaphore, sem_t **avail_semaphore, int *vision_opened, FILE **resultado_file);
void create_pipes_and_children(int parent_to_child_pipe[CHILD_QTY][2], int child_to_parent_pipe[CHILD_QTY][2], pid_t child_pid[CHILD_QTY]);
void handle_select_and_pipes(int argc, const char *argv[], int parent_to_child_pipe[CHILD_QTY][2], int child_to_parent_pipe[CHILD_QTY][2], int *files_assigned, pid_t child_pid[CHILD_QTY], char child_md5[CHILD_QTY][MAX_MD5 + MAX_PATH + 4], sem_t *shm_semaphore, sem_t *avail_semaphore, char *shared_memory, FILE *resultado_file, int *vision_opened);
void cleanup(sem_t *shm_semaphore, sem_t *avail_semaphore, int shm_fd, pid_t child_pid[CHILD_QTY], int parent_to_child_pipe[CHILD_QTY][2]);
int distribute_initial_files(int argc, const char *argv[], int parent_to_child_pipe[][2], int child_to_parent_pipe[][2]);


int main(int argc, const char *argv[]) {
    int vision_opened = 0;
    int shm_fd;
    char *shared_memory;
     if (!isatty(STDOUT_FILENO)) {
        pipe_write(STDOUT_FILENO, SHARED_MEMORY_NAME);
    }
    sem_t *shm_semaphore;
    sem_t *avail_semaphore;
    FILE *resultado_file;
    pid_t child_pid[CHILD_QTY];
    int parent_to_child_pipe[CHILD_QTY][2];
    int child_to_parent_pipe[CHILD_QTY][2];
    char child_md5[CHILD_QTY][MAX_MD5 + MAX_PATH + 4];
    int files_assigned;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    initialize_resources(&shm_fd, &shared_memory, &shm_semaphore, &avail_semaphore, &vision_opened, &resultado_file);
    create_pipes_and_children(parent_to_child_pipe, child_to_parent_pipe, child_pid);
    files_assigned = distribute_initial_files(argc, argv, parent_to_child_pipe, child_to_parent_pipe);
    handle_select_and_pipes(argc, argv, parent_to_child_pipe, child_to_parent_pipe, &files_assigned, child_pid, child_md5, shm_semaphore, avail_semaphore, shared_memory, resultado_file, &vision_opened);
    cleanup(shm_semaphore, avail_semaphore, shm_fd, child_pid, parent_to_child_pipe);

    return 0;
}

void initialize_resources(int *shm_fd, char **shared_memory, sem_t **shm_semaphore, sem_t **avail_semaphore, int *vision_opened, FILE **resultado_file) {
    
    *shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (*shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    *shared_memory = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, *shm_fd, 0);
    if (*shared_memory == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    sleep(2);

    *shm_semaphore = sem_open(SHM_SEMAPHORE_NAME, 1); // Open existing semaphore
    if (*shm_semaphore == SEM_FAILED) {
        perror("sem_open (shm_semaphore)");
        printf("No view detected\n");
    } else {
        (*vision_opened)++;
    }

    *avail_semaphore = sem_open(AVAIL_SEMAPHORE_NAME, 0); // Open existing semaphore
    if (*avail_semaphore == SEM_FAILED) {
        perror("sem_open (avail_semaphore)");
        printf("No view detected\n");
    } else {
        (*vision_opened)++;
    }

    if (*vision_opened == 1) {
        perror("missing 1 semaphore");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(*shm_fd, SHARED_MEMORY_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    *resultado_file = fopen("resultado.txt", "wr");
    if (*resultado_file == NULL) {
        perror("fopen(resultado.txt)");
        exit(EXIT_FAILURE);
    }
}

void create_pipes_and_children(int parent_to_child_pipe[CHILD_QTY][2], int child_to_parent_pipe[CHILD_QTY][2], pid_t child_pid[CHILD_QTY]) {
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

            dup2(parent_to_child_pipe[i][0], STDIN_FILENO);
            close(parent_to_child_pipe[i][0]);

            dup2(child_to_parent_pipe[i][1], STDOUT_FILENO);
            close(child_to_parent_pipe[i][1]);

            char *args[] = {"./slave.elf", NULL};
            execve(args[0], args, NULL);
            perror("execve");
            
            //when returned, all files were hased. So now we gotta close the child's file descriptors
            
            exit(EXIT_SUCCESS);
        }
    }
}

void handle_select_and_pipes(int argc, const char *argv[], int parent_to_child_pipe[CHILD_QTY][2], int child_to_parent_pipe[CHILD_QTY][2], 
int *files_assigned, pid_t child_pid[CHILD_QTY], char child_md5[CHILD_QTY][MAX_MD5 + MAX_PATH + 4], sem_t *shm_semaphore, sem_t *avail_semaphore, 
char *shared_memory, FILE *resultado_file, int *vision_opened) {
    fd_set readfds;
    int max_fd = -1;
    int current_file_index = 0;
    int total_files_to_process=argc-1;
    int info_length = strlen(INFO_TEXT) + MAX_MD5 + MAX_PATH + 2;

    for (int i = 0; i < CHILD_QTY; i++) {
        if (child_to_parent_pipe[i][0] > max_fd) {
            max_fd = child_to_parent_pipe[i][0];
        }
        FD_SET(child_to_parent_pipe[i][0], &readfds);
    }

    while (current_file_index < total_files_to_process) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int max_fd = -1;

        for (int i = 0; i < CHILD_QTY; i++) {
            if (child_to_parent_pipe[i][0] > max_fd) {
                max_fd = child_to_parent_pipe[i][0];
            }
            FD_SET(child_to_parent_pipe[i][0], &readfds);
        }

        int ready = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if (*vision_opened == 2) {
            sem_wait(shm_semaphore);
        }

        for (int i = 0; i < CHILD_QTY; i++) {
            if (FD_ISSET(child_to_parent_pipe[i][0], &readfds)) {
                int bytes_read = pipe_read(child_to_parent_pipe[i][0], child_md5[i]);
                if (bytes_read < 0) {
                    perror("read");
                    exit(EXIT_FAILURE);
                } else if (bytes_read == 0) {
                    close(child_to_parent_pipe[i][0]);
                    FD_CLR(child_to_parent_pipe[i][0], &readfds);
                } else {
                    fprintf(resultado_file, INFO_TEXT "\n", child_pid[i], child_md5[i]);
                    //fprintf(resultado_file, INFO_TEXT "\n", child_pid[i], child_md5[i]);
                    fflush(resultado_file);
                    snprintf(shared_memory + current_file_index * info_length, info_length, INFO_TEXT, child_pid[i], child_md5[i]);
                    //sprintf(shared_memory + current_file_index * info_length, INFO_TEXT, child_pid[i], child_md5[i], argv[current_file_index + 1]);   
                    //sprintf(shared_memory + current_file_index * info_length, INFO_TEXT, child_pid[i], child_md5[i]);

                    if (*files_assigned < argc) {
                        pipe_write(parent_to_child_pipe[i][1], argv[(*files_assigned)++]);
                    }

                    current_file_index++;
                }
            }
        }

        sprintf(shared_memory + current_file_index * info_length, "\n");
        if (*vision_opened == 2) {
            sem_post(shm_semaphore);
            sem_post(avail_semaphore);
        }
    }
    sprintf(shared_memory + current_file_index * info_length, "\t");
}

void cleanup(sem_t *shm_semaphore, sem_t *avail_semaphore, int shm_fd, pid_t child_pid[CHILD_QTY], int parent_to_child_pipe[CHILD_QTY][2]) {
    for (int i = 0; i < CHILD_QTY; i++) {
        close(parent_to_child_pipe[i][1]);
    }
    
    for(int i=0; i<CHILD_QTY; i++){
        waitpid(child_pid[i], NULL, 0);
    }
    
    sem_close(shm_semaphore);
    sem_unlink(SHM_SEMAPHORE_NAME);

    sem_close(avail_semaphore);
    sem_unlink(AVAIL_SEMAPHORE_NAME);

    close(shm_fd);
    shm_unlink(SHARED_MEMORY_NAME);
}

int distribute_initial_files(int argc, const char *argv[], int parent_to_child_pipe[][2], int child_to_parent_pipe[][2]) {
    
    int total_files_to_process=argc-1;

    int files_assigned = 1;
    for(int i=0; i<INITIAL_FILES_PER_CHILD && i*CHILD_QTY<total_files_to_process; i++){
        for(int child_index=0; child_index<CHILD_QTY && i*CHILD_QTY+child_index<total_files_to_process; child_index++){
            pipe_write(parent_to_child_pipe[child_index][1], argv[files_assigned++]);
        }
    }
    return files_assigned;
}
