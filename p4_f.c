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
#define FIFO_P4_P2 "/tmp/fifo_p4_p2"

typedef struct {

    int value;
} shared_data;

#include <sys/wait.h>

int main() {
    int mem;
    shared_data *data;

    //Abre sem
    sem_t *sem_1 = sem_open(SEM_EMPTY, 0);
    sem_t *sem_2 = sem_open(SEM_FULL, 0);
    sem_t *sem_3 = sem_open(SEM_MUTEX, 0);

    if (sem_1 == SEM_FAILED || sem_2 == SEM_FAILED || sem_3 == SEM_FAILED) {
        perror("Error al abrir semáforos");
        exit(EXIT_FAILURE);
    }

    // Conectarse a la memoria compartida creada por P3
    mem = shm_open(SHM_NOM, O_RDWR, 0666);
    if (mem == -1) {
        perror("Error al abrir memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Mapear la memoria compartida
    data = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, mem, 0);
    if (data == MAP_FAILED) {
        perror("Error mmap");
        exit(EXIT_FAILURE);
    }

    while (1) {
        sem_wait(sem_2);
        sem_wait(sem_3);

        int val = data->value;

        sem_post(sem_3);
        sem_post(sem_1);

        // Si recibe el testigo de fin de P2 (-2), debe notificarle con -3 mediante FIFO
        if (val == -1 || val == -2) {

            printf("-3 P4 termina\n");

            int fifo_fd;
            int msg = -3;
            ssize_t bytes_written = write(fifo_fd, &msg, sizeof(int));

            if (bytes_written != sizeof(int)) {

                perror("Error al escribir en FIFO");
            }
    }

    munmap(data, sizeof(shared_data));
    close(mem);

    sem_close(sem_1);
    sem_close(sem_2);
    sem_close(sem_3);

    return 0;
}
}