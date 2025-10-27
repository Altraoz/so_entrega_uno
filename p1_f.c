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
#include <sys/wait.h>


#define SHM "/shm"

#define SEM_EMPTY "/sem_empty"
#define SEM_FULL "/sem_full"
#define SEM_MUTEX "/sem_mutex"

#define SEM_TURN_P1 "/sem_turn_p1"
#define SEM_TURN_P2 "/sem_turn_p2"
#define SEM_TURN_P3 "/sem_turn_p3"
#define SEM_TURN_P4 "/sem_turn_p4"

#define SEM_P3_READY "/sem_p3_ready"
#define SEM_P4_READY "/sem_p4_ready"



typedef struct { int value; } shared_data;

static int parse_int(const char *arg, const char *name, long lo, long hi) {
    char *end = NULL; errno = 0;
    long v = strtol(arg, &end, 10);
    if (errno || *end || v < lo || v > hi) {
        fprintf(stderr, "%s debe ser entero valido [%ld..%ld]\n", name, lo, hi);
        exit(1);
    }
    return (int)v;
}

// p1: algoritmo de Fibonacci
static void run_fibo(int a1, int a2, int N,
    shared_data *buf,
    sem_t *empty, sem_t *full, sem_t *mutex, sem_t *turn_p1, sem_t *turn_p3) {
    int prev = a1;
    int curr = a2;

    for (int i = 0; i < N; i++) {
        int next = prev + curr;
        sem_wait(turn_p1);
        sem_wait(empty);
        sem_wait(mutex);
        buf->value = next;
        sem_post(turn_p3);
        sem_post(mutex);
        sem_post(full);
        prev = curr;
        curr = next;
    }

    sem_wait(turn_p1);
    sem_wait(empty);
    sem_wait(mutex);
    buf->value = -1;
    sem_post(turn_p3);
    sem_post(mutex);
    sem_post(full);

    int fd = open("/tmp/fifo_p1", O_RDONLY);
    if (fd != -1) {
        int msg; read(fd, &msg, sizeof msg); close(fd);
        if (msg == -3) printf("-3 P1 termina\n");
    }
}

// p2: algoritmo de potencias
static void run_pow(int a3, int N,
                    shared_data *buf,
                    sem_t *empty, sem_t *full, sem_t *mutex, sem_t *turn_p2, sem_t *turn_p4) {

    for (int i = 0; i < N; i++) {
        int val = 1 << (a3 + i);
        sem_wait(turn_p2);
        sem_wait(empty);
        sem_wait(mutex);
        buf->value = val;
        sem_post(turn_p4);
        sem_post(mutex);
        sem_post(full);
    }

    sem_wait(turn_p2);
    sem_wait(empty);
    sem_wait(mutex);
    buf->value = -2;
    sem_post(turn_p4);
    sem_post(mutex);
    sem_post(full);

    int fd = open("/tmp/fifo_p2", O_RDONLY);
    if (fd != -1) {
        int msg; read(fd, &msg, sizeof msg); close(fd);
        if (msg == -3) printf("-3 P2 termina\n");
    }
}

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s N a1 a2 a3\n", argv[0]);
        return 1;
    }
    const int N  = parse_int(argv[1], "N", 1, INT_MAX);
    const int a1 = parse_int(argv[2], "a1", INT_MIN, INT_MAX);
    const int a2 = parse_int(argv[3], "a2", INT_MIN, INT_MAX);
    const int a3 = parse_int(argv[4], "a3", 0, 30);

    int shm = shm_open(SHM, O_RDWR, 0666);
    if (shm == -1) {fprintf(stderr, "P3 no está en ejecución\n"); exit(1);}
    shared_data *buffer = mmap(NULL, sizeof(shared_data),
                              PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (buffer == MAP_FAILED) {fprintf(stderr, "P3 no está en ejecución\n"); close(shm); exit(1);}

    sem_t *empty = sem_open(SEM_EMPTY, 0);
    sem_t *full  = sem_open(SEM_FULL,  0);
    sem_t *mutex = sem_open(SEM_MUTEX, 0);
    sem_t *turn_p1 = sem_open(SEM_TURN_P1, 0);
    sem_t *turn_p2 = sem_open(SEM_TURN_P2, 0);
    sem_t *turn_p3 = sem_open(SEM_TURN_P3, 0);
    sem_t *turn_p4 = sem_open(SEM_TURN_P4, 0);

    if (empty==SEM_FAILED || full==SEM_FAILED || mutex==SEM_FAILED ||
        turn_p1==SEM_FAILED || turn_p2==SEM_FAILED ||
        turn_p3==SEM_FAILED || turn_p4==SEM_FAILED) {
        fprintf(stderr, "P3 o P4 no están en ejecución\n");
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

    sem_t *sem_p4_ready = sem_open(SEM_P4_READY, 0);
    if (sem_p4_ready==SEM_FAILED) {
        fprintf(stderr, "P4 no están en ejecución\n");
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

    pid_t pid = fork();

    if (pid < 0) {
        perror("p1 fork");
        munmap(buffer, sizeof(shared_data));
        close(shm);
        sem_close(empty);
        sem_close(full);
        sem_close(mutex);
        sem_close(turn_p1);
        sem_close(turn_p2);
        sem_close(turn_p3);
        sem_close(turn_p4);
        sem_close(sem_p4_ready);
        exit(1);
    }

    if (pid == 0) {
        sem_post(turn_p2);
        run_pow(a3, N, buffer, empty, full, mutex, turn_p2, turn_p4);
        munmap(buffer, sizeof(shared_data));
        close(shm);
        sem_close(empty);
        sem_close(full);
        sem_close(mutex);
        sem_close(turn_p1);
        sem_close(turn_p2);
        sem_close(turn_p3);
        sem_close(turn_p4);
        sem_close(sem_p4_ready);
        _exit(0);
    } else {
        sem_post(turn_p1);
        run_fibo(a1, a2, N, buffer, empty, full, mutex, turn_p1, turn_p3);
        int st; waitpid(pid, &st, 0);
        munmap(buffer, sizeof(shared_data));
        close(shm);
        sem_close(empty);
        sem_close(full);
        sem_close(mutex);
        sem_close(turn_p1);
        sem_close(turn_p2);
        sem_close(turn_p3);
        sem_close(turn_p4);
        sem_close(sem_p4_ready);
    }
    return 0;
}
