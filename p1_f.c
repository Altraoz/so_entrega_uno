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

#define SHM_FIBO      "/shm_fibo"
#define SEM_F_EMPTY   "/sem_fibo_empty"
#define SEM_F_FULL    "/sem_fibo_full"
#define SEM_F_MUTEX   "/sem_fibo_mutex"

#define SEM_TURN_P1   "/sem_turn_p1"
#define SEM_TURN_P2   "/sem_turn_p2"
#define SEM_TURN_P3   "/sem_turn_p3"
#define SEM_TURN_P4   "/sem_turn_p4"

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

// ----- P1: Fibonacci -----
// ----- P1: Fibonacci (emite N términos comenzando en a1+a2) -----
static void run_fibo(int a1, int a2, int N,
    shared_data *buf,
    sem_t *empty, sem_t *full, sem_t *mutex, sem_t *turn_p1, sem_t *turn_p3) {
    int prev = a1;
    int curr = a2;


    //EJECUTAR SI ES SU TURNO, LUEGO CEDERLO A P3

    for (int i = 0; i < N; i++) {
        int next = prev + curr;   // primer término a emitir
        // printf("F(%d) = %d\n", i, next); // opcional: etiqueta de depuración

        sem_wait(empty);
        sem_wait(mutex);
        sem_wait(turn_p1);  // Esperar turno de P1
        printf("Hola desde p1\n");
        buf->value = next;
        sem_post(mutex);
        sem_post(full);
        sem_post(turn_p3);  // Ceder turno a P3

        prev = curr;
        curr = next;
    }

    // testigo -1 para P3
    sem_wait(empty);
    sem_wait(mutex);
    sem_wait(turn_p1);  // Esperar turno de P1
    buf->value = -1;
    sem_post(mutex);
    sem_post(full);
    sem_post(turn_p3);  // Ceder turno a P3

    // Espera -3 de P3
    int fd = open("/tmp/fifo_p1", O_RDONLY);
    if (fd != -1) {
        int msg; read(fd, &msg, sizeof msg); close(fd);
        if (msg == -3) printf("-3 P1 termina\n");
    }
}

// ----- P2: Potencias -----
static void run_pow(int a3, int N,
                    shared_data *buf,
                    sem_t *empty, sem_t *full, sem_t *mutex, sem_t *turn_p2, sem_t *turn_p4) {

    //EJECUTAR SI ES TURNO DE P2, LUEGO CEDERLO A P4


    for (int i = 0; i < N; i++) {
        int val = 1 << (a3 + i);
        sem_wait(empty);
        sem_wait(mutex);
        sem_wait(turn_p2);  // Esperar turno de P2
        
        printf("Hola desde p2\n");
        buf->value = val;
        sem_post(mutex);
        sem_post(full);
        sem_post(turn_p4);  // Ceder turno a P4
    }

    // testigo -2 para P4
    sem_wait(empty);
    sem_wait(mutex);
    sem_wait(turn_p2);  // Esperar turno de P2
    buf->value = -2;
    sem_post(mutex);
    sem_post(full);
    sem_post(turn_p4);  // Ceder turno a P4

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

    sem_t *turn_p1 = sem_open(SEM_TURN_P1, 0);
    sem_t *turn_p2 = sem_open(SEM_TURN_P2, 0);
    sem_t *turn_p3 = sem_open(SEM_TURN_P3, 0);
    sem_t *turn_p4 = sem_open(SEM_TURN_P4, 0);

    // 1) ABRIR recursos (creados por P3)
    int shm_f = shm_open(SHM_FIBO, O_RDWR, 0666);
    if (shm_f == -1) { perror("p1 shm_open fibo (¿p3 no corre?)"); exit(1); }
    shared_data *buf_f = mmap(NULL, sizeof(shared_data),
                              PROT_READ | PROT_WRITE, MAP_SHARED, shm_f, 0);
    if (buf_f == MAP_FAILED) { perror("p1 mmap fibo"); exit(1); }
    sem_t *f_empty = sem_open(SEM_F_EMPTY, 0);
    sem_t *f_full  = sem_open(SEM_F_FULL,  0);
    sem_t *f_mutex = sem_open(SEM_F_MUTEX, 0);
    if (f_empty==SEM_FAILED || f_full==SEM_FAILED || f_mutex==SEM_FAILED) {
        perror("p1 sem_open fibo"); exit(1);
    }

    // 3) Validar que P3 y P4 están en ejecución con sem_getvalue (requisito)
    int v1, v2, v3;
    if (sem_getvalue(f_empty, &v1) || sem_getvalue(f_full, &v2) || sem_getvalue(f_mutex, &v3)) {
        perror("p1 sem_getvalue"); exit(1);
    }

    // 4) Crear P2 y ejecutar ambos productores en paralelo (o secuencial si prefieres)
    pid_t pid = fork();

    printf("[PRE] empty=%p mutex=%p turn_p2=%p\n", (void*)f_empty, (void*)f_mutex, (void*)turn_p2);
    fflush(stdout);
    if (pid < 0) { perror("p1 fork"); exit(1); }

    if (pid == 0) {
        printf("Hola mundo1\n");

        // Hijo → P2: produce Potencias
        run_pow(a3, N, buf_f, f_empty, f_full, f_mutex, turn_p2, turn_p4);
        _exit(0);
    } else {
        printf("Hola mundo2\n");

        // Padre → P1: produce Fibonacci
        run_fibo(a1, a2, N, buf_f, f_empty, f_full, f_mutex, turn_p1, turn_p3);
        int st; waitpid(pid, &st, 0);
        // Limpieza local
        munmap(buf_f, sizeof(shared_data)); close(shm_f);
        sem_close(f_empty); sem_close(f_full); sem_close(f_mutex);
    }
    return 0;
}