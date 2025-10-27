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

#define SHM "/shm"

#define SEM_EMPTY "/sem_empty"
#define SEM_FULL "/sem_full"
#define SEM_MUTEX "/sem_mutex"

#define SEM_TURN_P1 "/sem_turn_p1"
#define SEM_TURN_P2 "/sem_turn_p2"
#define SEM_TURN_P3 "/sem_turn_p3"
#define SEM_TURN_P4 "/sem_turn_p4"

typedef struct { int value; } shared_data;

int main() {
    // abrir shm y buffer
    int shm = shm_open(SHM, O_RDWR, 0666);
    if (shm == -1) { perror("p1 shm_open fibo (¿p3 no corre?)"); exit(1); }
    shared_data *buffer = mmap(NULL, sizeof(shared_data),
                              PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (buffer == MAP_FAILED) { perror("p1 mmap fibo"); exit(1); }

    // inicializar semáforos
    sem_t *empty = sem_open(SEM_EMPTY, 0);
    sem_t *full  = sem_open(SEM_FULL,  0);
    sem_t *mutex = sem_open(SEM_MUTEX, 0);
    sem_t *turn_p1 = sem_open(SEM_TURN_P1, 0);
    sem_t *turn_p2 = sem_open(SEM_TURN_P2, 0);
    sem_t *turn_p3 = sem_open(SEM_TURN_P3, 0);
    sem_t *turn_p4 = sem_open(SEM_TURN_P4, 0);
    if (empty==SEM_FAILED || full==SEM_FAILED || mutex==SEM_FAILED ç|| 
        turn_p1==SEM_FAILED || turn_p2==SEM_FAILED || turn_p3==SEM_FAILED || turn_p4==SEM_FAILED) {
        perror("p4 sem_open"); exit(1);
    }
    printf("Esperando P2\n");


    //consumir p2
    while (1) {
        sem_wait(turn_p4);  // esperar turno de p4
        sem_wait(full);
        sem_wait(mutex);

        int val = buffer->value;

        sem_post(turn_p1);  // ceder turno a p1
        sem_post(mutex);
        sem_post(empty);

        if (val == -2) {
            // notificar a p2 por FIFO con -3
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

        // mostrar valor leído
        printf("%d ", val);
        fflush(stdout);
    }

    // limpieza
    munmap(buffer, sizeof(shared_data));
    close(shm);
    sem_close(empty); sem_close(full); sem_close(mutex);
    return 0;
}
