//intento
//asdasd
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#define SHM_NOM "/mem_compartida"
#define SEM_EMPTY "/sem_1" // /sem_empty
#define SEM_FULL "/sem_2"   // /sem_full
#define SEM_MUTEX "/sem_3" // /sem_mutex

typedef struct {

    int value;
} shared_data;

#include <sys/wait.h>

int main() {
    int mem; //shm_fd
    shared_data *data;
    sem_t *sem_1, *sem_2, *sem_3;

// la memoria

    mem = shm_open(SHM_NOM, O_CREAT | O_RDWR, 0666);
    if (mem == -1) { 

        perror("Error shm_open"); 
        exit(1); 
    }

    if (ftruncate(mem, sizeof(shared_data)) == -1) {

        perror("Erorr ftruncate"); 
        exit(1);
    }

    data=mmap(NULL, sizeof(shared_data), PROT_READ|PROT_WRITE, MAP_SHARED, mem, 0);
    if (data == MAP_FAILED) { 

        perror("Error mmap"); 
        exit(1); 
    }

    sem_1 = sem_open(SEM_EMPTY, O_CREAT, 0666, 1);
    sem_2  = sem_open(SEM_FULL,  O_CREAT, 0666, 0);
    sem_3 = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);

    if (sem_1 == SEM_FAILED || sem_2 == SEM_FAILED || sem_3 == SEM_FAILED) {

        perror("Error sem_open"); 
        exit(1);
    }

   //printf("Esperando P1\n");
    while (1) {

        sem_wait(sem_2);
        sem_wait(sem_3);

        int val = data->value;

        sem_post(sem_3);
        sem_post(sem_1);

        // Si el buffer recibe el testigo -1, p3 debe enviar -3 a p1 mediante FIFO
        if (val == -1) {          // testigo de P1
            mkfifo("/tmp/fifo_p1", 0666);               // (hacerlo una vez)
            int fd = open("/tmp/fifo_p1", O_WRONLY);
            int msg = -3;
            write(fd, &msg, sizeof msg);
            close(fd);
            printf("-3 P3 termina\n");
            break;
        }
         // imprimir solo lo que te corresponda (Fibonacci) 
        printf("%d ", val);
        fflush(stdout);

    }    
    munmap(data, sizeof(shared_data));
    close(mem);
    sem_close(sem_1);
    sem_close(sem_2);
    sem_close(sem_3);
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_MUTEX);
    shm_unlink(SHM_NOM);

    return 0;
}