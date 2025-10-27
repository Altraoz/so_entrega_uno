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

#define SEM_P4_READY "/sem_p4_ready"

typedef struct { int value; } shared_data;

int main() {
    // abrir shm y buffer
    int shm = shm_open(SHM, O_RDWR, 0666);
    if (shm == -1)  {fprintf(stderr, "P3 no está en ejecución\n"); exit(1);}
    shared_data *buffer = mmap(NULL, sizeof(shared_data),
                              PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (buffer == MAP_FAILED) {fprintf(stderr, "P3 no está en ejecución\n"); close(shm); exit(1);}

    // inicializar semáforos
    sem_t *empty = sem_open(SEM_EMPTY, 0);
    sem_t *full  = sem_open(SEM_FULL,  0);
    sem_t *mutex = sem_open(SEM_MUTEX, 0);
    sem_t *turn_p1 = sem_open(SEM_TURN_P1, 0);
    sem_t *turn_p2 = sem_open(SEM_TURN_P2, 0);
    sem_t *turn_p3 = sem_open(SEM_TURN_P3, 0);
    sem_t *turn_p4 = sem_open(SEM_TURN_P4, 0);
    if (empty==SEM_FAILED || full==SEM_FAILED || mutex==SEM_FAILED || turn_p1==SEM_FAILED || turn_p2==SEM_FAILED || turn_p3==SEM_FAILED || turn_p4==SEM_FAILED) {
        fprintf(stderr, "p3 no está en ejecución\n");
        munmap(buffer, sizeof(shared_data));
        close(shm);
        if (empty!=SEM_FAILED) sem_close(empty);
        if (full!=SEM_FAILED) sem_close(full);
        if (mutex!=SEM_FAILED) sem_close(mutex);
        if (turn_p1!=SEM_FAILED) sem_close(turn_p1);
        if (turn_p2!=SEM_FAILED) sem_close(turn_p2);
        if (turn_p3!=SEM_FAILED) sem_close(turn_p3);
        if (turn_p4!=SEM_FAILED) sem_close(turn_p4);
        exit(1);
    }
    printf("Esperando P2\n");

    // semaforo de estado proceso p4
    sem_t *p4_ready = sem_open(SEM_P4_READY, O_CREAT, 0666, 1);
    if (p4_ready == SEM_FAILED) {
        fprintf(stderr, "error creando semáforo p4_ready\n");
        munmap(buffer, sizeof(shared_data));
        close(shm);
        sem_close(empty);
        sem_close(full);
        sem_close(mutex);
        sem_close(turn_p1);
        sem_close(turn_p2);
        sem_close(turn_p3);
        sem_close(turn_p4);
        exit(1);
    }

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
    sem_close(p4_ready);
    sem_close(empty); sem_close(full); sem_close(mutex);
    sem_close(turn_p1);
    sem_close(turn_p2);
    sem_close(turn_p3);
    sem_close(turn_p4);
    sem_unlink(SEM_P4_READY);
    return 0;
}
