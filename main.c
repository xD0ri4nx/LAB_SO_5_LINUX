#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>

#define SHM_NAME "/my_memory"
#define SEM_NAME "/my_semaphore"

typedef struct {
    int number; 
} shared_data_t;

int main() {
    int shm_fd;
    long pg_size = sysconf(_SC_PAGE_SIZE);
    void *virt_addr;
    shared_data_t *shared_data;
    sem_t *sem;

    
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failure");
        exit(EXIT_FAILURE);
    }

    
    if (ftruncate(shm_fd, pg_size) == -1) {
        perror("ftruncate failure");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

   
    virt_addr = mmap(NULL, pg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (virt_addr == MAP_FAILED) {
        perror("mmap failure");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    shared_data = (shared_data_t *)virt_addr;
    shared_data->number = 0; 

   
    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1); 
    if (sem == SEM_FAILED) {
        perror("sem_open failure");
        munmap(virt_addr, pg_size);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

 
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failure");
        sem_close(sem);
        sem_unlink(SEM_NAME);
        munmap(virt_addr, pg_size);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
       
        srand(time(NULL) ^ getpid());
        while (1) {
            sem_wait(sem); 

            if (shared_data->number >= 1000) {
                sem_post(sem);
                break;
            }

            if (rand() % 2 == 1) { 
                shared_data->number += 1;
                printf("Child process writes: %d\n", shared_data->number);
            }

            sem_post(sem); 
            usleep(10000); 
        }

        printf("Child process finished.\n");
    } else {
       
        srand(time(NULL) ^ getpid());
        while (1) {
            sem_wait(sem); 

            if (shared_data->number >= 1000) {
                sem_post(sem); 
                break;
            }

            if (rand() % 2 == 1) { 
                shared_data->number += 1;
                printf("Parent process writes: %d\n", shared_data->number);
            }

            sem_post(sem); 
            usleep(10000); 
        }

        
        wait(NULL);

        printf("Parent process finished.\n");

      
        sem_close(sem);
        sem_unlink(SEM_NAME);
        munmap(virt_addr, pg_size);
        close(shm_fd);
        shm_unlink(SHM_NAME);
    }

    return 0;
}
