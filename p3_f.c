// p3_f.c  (Consumidor Fibonacci, CREADOR de recursos Fibonacci)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

#define SHM_FIBO      "/shm_fibo"
#define SEM_F_EMPTY   "/sem_fibo_empty"
#define SEM_F_FULL    "/sem_fibo_full"
#define SEM_F_MUTEX   "/sem_fibo_mutex"

typedef struct { int value; } shared_data;

int main() {
    // 1) Crear SHM y semáforos SOLO para la tubería Fibonacci
    int shm_fd = shm_open(SHM_FIBO, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) { perror("p3 shm_open"); exit(1); }
    if (ftruncate(shm_fd, sizeof(shared_data)) == -1) { perror("p3 ftruncate"); exit(1); }

    shared_data *data = mmap(NULL, sizeof(shared_data),
                             PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) { perror("p3 mmap"); exit(1); }

    sem_t *empty = sem_open(SEM_F_EMPTY, O_CREAT, 0666, 1);
    sem_t *full  = sem_open(SEM_F_FULL,  O_CREAT, 0666, 0);
    sem_t *mutex = sem_open(SEM_F_MUTEX, O_CREAT, 0666, 1);
    if (empty == SEM_FAILED || full == SEM_FAILED || mutex == SEM_FAILED) {
        perror("p3 sem_open"); exit(1);
    }

    // 2) Consumir SOLO Fibonacci (producido por P1) hasta recibir -1
    while (1) {
        sem_wait(full);
        sem_wait(mutex);

        int val = data->value;

        sem_post(mutex);
        sem_post(empty);

        if (val == -1) {
            // Notificar a P1 por FIFO con -3
            mkfifo("/tmp/fifo_p1", 0666);
            int fd = open("/tmp/fifo_p1", O_WRONLY);
            if (fd != -1) {
                int msg = -3;
                write(fd, &msg, sizeof msg);
                close(fd);
            }
            printf("-3 P3 termina\n");
            break;
        }

        // Muestra SOLO Fibonacci
        printf("%d ", val);
        fflush(stdout);
    }

    // 3) Cierre / limpieza de RECURSOS Fibonacci
    munmap(data, sizeof(shared_data));
    close(shm_fd);
    sem_close(empty); sem_close(full); sem_close(mutex);

    // IMPORTANTE: No hagas unlink aquí si P1 sigue vivo y podría reabrir.
    // Solo el último en terminar debería hacer unlink. Para simplificar,
    // puedes dejar los unlink en un script de limpieza al final de todas las ejecuciones:
    // sem_unlink(SEM_F_EMPTY); sem_unlink(SEM_F_FULL); sem_unlink(SEM_F_MUTEX); shm_unlink(SHM_FIBO);

    return 0;
}
