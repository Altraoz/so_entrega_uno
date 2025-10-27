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


    //EJECUTAR SI ES SU TURNO, LUEGO CEDERLO A P3

    for (int i = 0; i < N; i++) {
        int next = prev + curr;   // primer término a emitir
        // printf("F(%d) = %d\n", i, next); // opcional: etiqueta de depuración
        sem_wait(turn_p1);  // Esperar turno de P1
        sem_wait(empty);
        sem_wait(mutex);
        buf->value = next;
        sem_post(turn_p3);  // Ceder turno a P3
        sem_post(mutex);
        sem_post(full);

        prev = curr;
        curr = next;
    }

    // testigo -1 para P3
    sem_wait(turn_p1);  // Esperar turno de P1
    sem_wait(empty);
    sem_wait(mutex);
    buf->value = -1;
    sem_post(turn_p3);  // Ceder turno a P3
    sem_post(mutex);
    sem_post(full);

    // Espera -3 de P3
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

    //EJECUTAR SI ES TURNO DE P2, LUEGO CEDERLO A P4


    for (int i = 0; i < N; i++) {

        // int v_empty, v_mutex, v_turn2;

        // sem_getvalue(empty, &v_empty);
        // sem_getvalue(mutex, &v_mutex);
        // sem_getvalue(turn_p2, &v_turn2);

        // printf("[PRE]%d empty=%d mutex=%d turn_p2=%d\n", i, v_empty, v_mutex, v_turn2);
        // fflush(stdout);


        int val = 1 << (a3 + i);
        sem_wait(turn_p2);  // Esperar turno de P2
        sem_wait(empty);
        sem_wait(mutex);
        
        buf->value = val;
        sem_post(turn_p4);  // Ceder turno a P4
        sem_post(mutex);
        sem_post(full);
    }

    // testigo -2 para P4
    sem_wait(turn_p2);  // Esperar turno de P2
    sem_wait(empty);
    sem_wait(mutex);
    buf->value = -2;
    sem_post(turn_p4);  // Ceder turno a P4
    sem_post(mutex);
    sem_post(full);

    // Espera -3 de P4
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
    const int a3 = parse_int(argv[4], "a3", 0, 30);  // 2^(a3+i) simple

    // abrir SHM y semáforos
    int shm = shm_open(SHM, O_RDWR, 0666);
    if (shm == -1) {fprintf(stderr, "P3 o P4 no están en ejecución\n"); exit(1);}
    shared_data *buffer = mmap(NULL, sizeof(shared_data),
                              PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (buffer == MAP_FAILED) {fprintf(stderr, "P3 o P4 no están en ejecución\n"); exit(1);}
    sem_t *empty = sem_open(SEM_EMPTY, 0);
    sem_t *full  = sem_open(SEM_FULL,  0);
    sem_t *mutex = sem_open(SEM_MUTEX, 0);
    sem_t *turn_p1 = sem_open(SEM_TURN_P1, 0);
    sem_t *turn_p2 = sem_open(SEM_TURN_P2, 0);
    sem_t *turn_p3 = sem_open(SEM_TURN_P3, 0);
    sem_t *turn_p4 = sem_open(SEM_TURN_P4, 0);
    if (empty==SEM_FAILED || full==SEM_FAILED || mutex==SEM_FAILED ||
        turn_p1==SEM_FAILED || turn_p2==SEM_FAILED) {
        fprintf(stderr, "P3 o P4 no están en ejecución\n"); exit(1);
    }

    // validar que p3 y p4 están en ejecución
    if (turn_p3==SEM_FAILED || turn_p4==SEM_FAILED) {
        fprintf(stderr, "P3 o P4 no están en ejecución\n"); exit(1);
    }


    // crear p2 y ejecutar
    pid_t pid = fork();

    if (pid < 0) { perror("p1 fork"); exit(1); }

    if (pid == 0) {
        // ejecutar p2
        printf("Child process running p2\n");
        sem_wait(turn_p1);
        run_pow(a3, N, buffer, empty, full, mutex, turn_p2, turn_p4);
        _exit(0);
    } else {
        // ejecutar p1
        printf("Child process running p1\n");
        sem_wait(turn_p2);
        run_fibo(a1, a2, N, buffer, empty, full, mutex, turn_p1, turn_p3);
        int st; waitpid(pid, &st, 0);
        // limpieza
        munmap(buffer, sizeof(shared_data)); close(shm);
        sem_close(empty); sem_close(full); sem_close(mutex);
    }
    return 0;
}