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
#define SHARED_MEMORY_SIZE 1024 // Same size as in md5.c

int main() {
    // Open shared memory and semaphores
    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    sem_t *app_semaphore = sem_open(APP_SEMAPHORE_NAME, 0);
    sem_t *view_semaphore = sem_open(VIEW_SEMAPHORE_NAME, 0);

    // Check for errors when opening shared memory and semaphores

    while (1) {
        // Wait for the application to write data to shared memory
        sem_wait(view_semaphore);

        // Map the shared memory segment into memory
        char *shared_memory = mmap(NULL, sizeof(char) * SHARED_MEMORY_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);

        if (shared_memory == MAP_FAILED) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }

        printf("Received from application: %s\n", shared_memory);

        // Unmap the shared memory
        munmap(shared_memory, sizeof(char) * SHARED_MEMORY_SIZE);

        // Signal the application that data has been read
        sem_post(app_semaphore);

        // Sleep or perform other processing as necessary
        usleep(100000); // Sleep for 100 milliseconds
    }

    // Cleanup: Close shared memory and semaphores
    close(shm_fd);
    sem_close(app_semaphore);
    sem_close(view_semaphore);

    return EXIT_SUCCESS;
}
