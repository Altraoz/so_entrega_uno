// p4_f.c  (Consumidor Potencias, CREADOR de recursos Potencias)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

#define SHM_POW      "/shm_pow"
#define SEM_P_EMPTY  "/sem_pow_empty"
#define SEM_P_FULL   "/sem_pow_full"
#define SEM_P_MUTEX  "/sem_pow_mutex"

typedef struct { int value; } shared_data;

int main() {
    // 1) Crear SHM y semáforos SOLO para la tubería Potencias
    int shm_fd = shm_open(SHM_POW, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) { perror("p4 shm_open"); exit(1); }
    if (ftruncate(shm_fd, sizeof(shared_data)) == -1) { perror("p4 ftruncate"); exit(1); }

    shared_data *data = mmap(NULL, sizeof(shared_data),
                             PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) { perror("p4 mmap"); exit(1); }

    sem_t *empty = sem_open(SEM_P_EMPTY, O_CREAT, 0666, 1);
    sem_t *full  = sem_open(SEM_P_FULL,  O_CREAT, 0666, 0);
    sem_t *mutex = sem_open(SEM_P_MUTEX, O_CREAT, 0666, 1);
    if (empty == SEM_FAILED || full == SEM_FAILED || mutex == SEM_FAILED) {
        perror("p4 sem_open"); exit(1);
    }

    // 2) Consumir SOLO Potencias (producido por P2) hasta recibir -2
    while (1) {
        sem_wait(full);
        sem_wait(mutex);

        int val = data->value;

        sem_post(mutex);
        sem_post(empty);

        if (val == -2) {
            // Notificar a P2 por FIFO con -3
            mkfifo("/tmp/fifo_p2", 0666);
            int fd = open("/tmp/fifo_p2", O_WRONLY);
            if (fd != -1) {
                int msg = -3;
                write(fd, &msg, sizeof msg);
                close(fd);
            }
            printf("-3 P4 termina\n");
            break;
        }

        // Muestra SOLO Potencias
        printf("%d ", val);
        fflush(stdout);
    }

    // 3) Limpieza RECURSOS Potencias
    munmap(data, sizeof(shared_data));
    close(shm_fd);
    sem_close(empty); sem_close(full); sem_close(mutex);
    // Dejar unlink para el final si aún pueden estar abiertos:
    // sem_unlink(SEM_P_EMPTY); sem_unlink(SEM_P_FULL); sem_unlink(SEM_P_MUTEX); shm_unlink(SHM_POW);

    return 0;
}
