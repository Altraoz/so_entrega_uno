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

#define SHM_FIBO      "/shm_fibo"
#define SEM_F_EMPTY   "/sem_fibo_empty"
#define SEM_F_FULL    "/sem_fibo_full"
#define SEM_F_MUTEX   "/sem_fibo_mutex"

#define SEM_TURN_P1   "/sem_turn_p1"
#define SEM_TURN_P2   "/sem_turn_p2"
#define SEM_TURN_P3   "/sem_turn_p3"
#define SEM_TURN_P4   "/sem_turn_p4"

typedef struct { int value; } shared_data;

int main() {
    // 1) ABRIR recursos (creados por P3)
    sem_t *turn_p1 = sem_open(SEM_TURN_P1, 0);
    sem_t *turn_p2 = sem_open(SEM_TURN_P2, 0);
    sem_t *turn_p3 = sem_open(SEM_TURN_P3, 0);
    sem_t *turn_p4 = sem_open(SEM_TURN_P4, 0);


    int shm_f = shm_open(SHM_FIBO, O_RDWR, 0666);
    if (shm_f == -1) { perror("p1 shm_open fibo (¿p3 no corre?)"); exit(1); }
    shared_data *buf_f = mmap(NULL, sizeof(shared_data),
                              PROT_READ | PROT_WRITE, MAP_SHARED, shm_f, 0);
    if (buf_f == MAP_FAILED) { perror("p1 mmap fibo"); exit(1); }
    sem_t *empty = sem_open(SEM_F_EMPTY, 0);
    sem_t *full  = sem_open(SEM_F_FULL,  0);
    sem_t *mutex = sem_open(SEM_F_MUTEX, 0);
    if (empty==SEM_FAILED || full==SEM_FAILED || mutex==SEM_FAILED) {
        perror("p1 sem_open fibo"); exit(1);
    }
    printf("Esperando P2\n");


    // 2) Consumir SOLO Potencias (producido por P2) hasta recibir -2
    while (1) {
        sem_wait(full);
        sem_wait(mutex);
        sem_wait(turn_p4);  // Esperar turno de P4
        int val = buf_f->value;

        sem_post(mutex);
        sem_post(empty);
        sem_post(turn_p1);  // Ceder turno a P1

        if (val == -2) {
            // Notificar a P2 por FIFO con -3
            mkfifo("/tmp/fifo_p2", 0666);
            int fd = open("/tmp/fifo_p2", O_WRONLY);
            if (fd != -1) {
                int msg = -3;
                write(fd, &msg, sizeof msg);
                close(fd);
            }
            printf("\n-3 P4 termina\n");
            break;
        }

        // Muestra SOLO Potencias
        printf("%d ", val);
        fflush(stdout);
    }

    // 3) Limpieza RECURSOS Potencias
    munmap(buf_f, sizeof(shared_data));
    close(shm_f);
    sem_close(empty); sem_close(full); sem_close(mutex);
    // Dejar unlink para el final si aún pueden estar abiertos:
    // sem_unlink(SEM_P_EMPTY); sem_unlink(SEM_P_FULL); sem_unlink(SEM_P_MUTEX); shm_unlink(SHM_POW);

    return 0;
}
