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

// Function to initialize semaphores
sem_t *initializeSemaphore(const char *name, int value) {
    sem_t *sem = sem_open(name, O_CREAT, 0666, value);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    return sem;
}

// Function to create and map shared memory
char *createSharedMemory(int *shm_fd) {
    *shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (*shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    char *shared_memory = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, *shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return shared_memory;
}

// Function to read and display shared memory data
void readSharedMemory(sem_t *shm_semaphore, sem_t *avail_semaphore, char *shared_memory) {
    int info_length = 0;

    while (1) {
        sem_wait(avail_semaphore);
        
        sem_wait(shm_semaphore);
        while (shared_memory[info_length] != '\n' && shared_memory[info_length] != '\t') {
            int i = strlen(shared_memory + info_length) + 1;
            if (i > 1) {
                printf("%s\n", shared_memory + info_length);
            }
            info_length += i;
        }

        if (shared_memory[info_length] == '\t') {
            sem_post(shm_semaphore);
            break;
        }
        sem_post(shm_semaphore);
    }
}

int main() {
    sem_unlink(AVAIL_SEMAPHORE_NAME);
    sem_unlink(SHM_SEMAPHORE_NAME);

    // Declare and initialize semaphores
    sem_t *shm_semaphore = initializeSemaphore(SHM_SEMAPHORE_NAME, 1);
    sem_t *avail_semaphore = initializeSemaphore(AVAIL_SEMAPHORE_NAME, 0);

    int shm_fd;  // Declare shm_fd here

    char *shared_memory = createSharedMemory(&shm_fd);

    // Read and display shared memory data
    readSharedMemory(shm_semaphore, avail_semaphore, shared_memory);

    // Clean up
    close(shm_fd);
    sem_close(shm_semaphore);
    sem_close(avail_semaphore);

    return 0;
}
