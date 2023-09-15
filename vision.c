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
#define SHM_SEMAPHORE_NAME "/shm_semaphore"
#define AVAIL_SEMAPHORE_NAME "/avail_semaphore"
#define INFO_TEXT "PID:%d - FILE:%s - MD5:%s\n"

#define SHARED_MEMORY_SIZE 100000
#define MAX_MD5 32
#define MAX_PATH 80

int main() {
    sem_unlink(AVAIL_SEMAPHORE_NAME);
    sem_unlink(SHM_SEMAPHORE_NAME);

    // Declare and initialize semaphores
    sem_t *shm_semaphore = sem_open(SHM_SEMAPHORE_NAME, O_CREAT, 0666, 1);
    if (shm_semaphore == SEM_FAILED) {
        perror("sem_open (shm_semaphore)");
        exit(EXIT_FAILURE);
    }

    sem_t *avail_semaphore = sem_open(AVAIL_SEMAPHORE_NAME, O_CREAT | O_EXCL, 0666, 0); // initialli, avail_semaphore is 0
    if (avail_semaphore == SEM_FAILED) {
        perror("sem_open (avail_semaphore)");
        exit(EXIT_FAILURE);
    }

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

    int info_length=0;
    while (1) {
        // Wait for the application to write data to shared memory
        // Process the received data (MD5 hash)
        // char md5[MAX_MD5 + 1];
        // strncpy(md5, shared_memory, MAX_MD5);
        // md5[MAX_MD5] = '\0';
        
        sem_wait(avail_semaphore);
        
        sem_wait(shm_semaphore);
        while(shared_memory[info_length]!='\n' && shared_memory[info_length]!='\t'){
            // Display the received data
            int i = strlen(shared_memory+info_length)+1;
            if(i>1){
                printf("%s\n", shared_memory+info_length);
            }
            info_length+=i;
        }

        if(shared_memory[info_length]=='\t'){
            sem_post(shm_semaphore);
            break;
        }
        // Signal the application that data has been read
        sem_post(shm_semaphore);

    }

    // Cleanup: Close shared memory and semaphores
    close(shm_fd);
    sem_close(shm_semaphore);
    sem_close(avail_semaphore);

    return 0;
}