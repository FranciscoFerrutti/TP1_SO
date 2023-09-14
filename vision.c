//vision.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>

#define SHARED_MEMORY_NAME "/my_shared_memory"
#define APP_SEMAPHORE_NAME "/app_semaphore"
#define VIEW_SEMAPHORE_NAME "/view_semaphore"
#define SHARED_MEMORY_SIZE 100000
#define MAX_MD5 32

int main() {

  
    // Create shared memory
    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (shm_fd == -1) {
        perror("shm_open");
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
    
    char *shared_memory = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

 
    while (shared_memory[0]!= '\0') {
        // Wait for the application to write data to shared memory
        sem_wait(view_semaphore);

        // Process the received data (MD5 hash)
        // char md5[MAX_MD5 + 1];
        // strncpy(md5, shared_memory, MAX_MD5);
        // md5[MAX_MD5] = '\0';

        // Display the received data
        if(shared_memory[0] != '\0'){
            printf("%s\n", shared_memory);
        }
        

        // Signal the application that data has been read
        sem_post(app_semaphore);

    }

    // Cleanup: Close shared memory and semaphores
    close(shm_fd);
    sem_close(app_semaphore);
    sem_close(view_semaphore);

    return EXIT_SUCCESS;
}