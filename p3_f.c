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
    // crear shm y buffer
    int shm = shm_open(SHM, O_CREAT | O_RDWR, 0666);
    if (shm == -1) { perror("p3 shm_open"); exit(1); }
    if (ftruncate(shm, sizeof(shared_data)) == -1) { perror("p3 ftruncate"); exit(1); }

    shared_data *data = mmap(NULL, sizeof(shared_data),
                             PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (data == MAP_FAILED) { perror("p3 mmap"); exit(1); }

    // inicializar semáforos
    sem_t *empty = sem_open(SEM_EMPTY, O_CREAT, 0666, 1);
    sem_t *full  = sem_open(SEM_FULL,  O_CREAT, 0666, 0);
    sem_t *mutex = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);
    sem_t *turn_p1 = sem_open(SEM_TURN_P1, O_CREAT, 0666, 0);
    sem_t *turn_p2 = sem_open(SEM_TURN_P2, O_CREAT, 0666, 1);
    sem_t *turn_p3 = sem_open(SEM_TURN_P3, O_CREAT, 0666, 0);
    sem_t *turn_p4 = sem_open(SEM_TURN_P4, O_CREAT, 0666, 0);

    if (empty == SEM_FAILED || full == SEM_FAILED || 
        mutex == SEM_FAILED || turn_p1 == SEM_FAILED || 
        turn_p3 == SEM_FAILED || turn_p2 == SEM_FAILED || turn_p4 == SEM_FAILED) {
        perror("p3 sem_open"); exit(1);
    }
    printf("Esperando P1\n");

    //consumir p1
    while (1) {
        sem_wait(turn_p3);  // esperar turno de p3
        sem_wait(full);
        sem_wait(mutex);

        int val = data->value;
        sem_post(turn_p2);  // ceder turno a p2
        sem_post(mutex);
        sem_post(empty);

        // notificar a p1 por FIFO con -3
        if (val == -1) {
            mkfifo("/tmp/fifo_p1", 0666);
            int fd = open("/tmp/fifo_p1", O_WRONLY);
            if (fd != -1) {
                int msg = -3;
                write(fd, &msg, sizeof msg);
                close(fd);
            }
            printf("\n-3 P3 termina\n");
            break;
        }

        // mostrar valor leído
        printf("%d ", val);
        fflush(stdout);
    }

    /// limpieza
    munmap(data, sizeof(shared_data));
    close(shm);
    sem_close(empty); sem_close(full); sem_close(mutex);

    // limpieza
    munmap(data, sizeof(shared_data));
    close(shm);
    sem_close(empty);
    sem_close(full);
    sem_close(mutex);
    sem_close(turn_p1);
    sem_close(turn_p2);
    sem_close(turn_p3);
    sem_close(turn_p4);

    // eliminar semáforos y shm
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_TURN_P1);
    sem_unlink(SEM_TURN_P2);
    sem_unlink(SEM_TURN_P3);
    sem_unlink(SEM_TURN_P4);
    shm_unlink(SHM);


    return 0;
}
