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

#define SHM_POW      "/shm_pow"
#define SEM_P_EMPTY  "/sem_pow_empty"
#define SEM_P_FULL   "/sem_pow_full"
#define SEM_P_MUTEX  "/sem_pow_mutex"

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

void fibonacci(int a1, int a2, int N, shared_data *data,
               sem_t *sem_1, sem_t *sem_2, sem_t *sem_3) {
                
    int next;
    for (int i = 0; i < N; i++) {
        next = a1 + a2;
        printf("%d", next);
        if (i < N - 1) printf(", ");
        a1 = a2;
        a2 = next;
    }

    sem_wait(sem_1);
    sem_wait(sem_3);

    data->value = next;

    sem_post(sem_3);
    sem_post(sem_2);



    // testigo -1 para P3
    sem_wait(empty);
    sem_wait(mutex);
    buf->value = -1;
    sem_post(mutex);
    sem_post(full);

    // Espera -3 de P3
    int fd = open("/tmp/fifo_p1", O_RDONLY);
    if (fd != -1) {
        int msg; read(fd, &msg, sizeof msg); close(fd);
        if (msg == -3) printf("-3 P1 termina\n");
    }
}

// algoritmo de potencias
static void run_pow(int a3, int N,
                    shared_data *buf,
                    sem_t *empty, sem_t *full, sem_t *mutex) {

    for (int i = 0; i < N; i++) {
        int val = 1 << (a3 + i);

        sem_wait(empty);
        sem_wait(mutex);
        buf->value = val;
        sem_post(mutex);
        sem_post(full);
    }

    // testigo -2 para P4
    sem_wait(empty);
    sem_wait(mutex);
    buf->value = -2;
    sem_post(mutex);
    sem_post(full);

    // espera -3 de P4
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

    // apertura de recursos para Fibonacci
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

    // apertura de recursos para potencias
    int shm_p = shm_open(SHM_POW, O_RDWR, 0666);
    if (shm_p == -1) { perror("p1 shm_open pow (¿p4 no corre?)"); exit(1); }
    shared_data *buf_p = mmap(NULL, sizeof(shared_data),
                              PROT_READ | PROT_WRITE, MAP_SHARED, shm_p, 0);
    if (buf_p == MAP_FAILED) { perror("p1 mmap pow"); exit(1); }
    sem_t *p_empty = sem_open(SEM_P_EMPTY, 0);
    sem_t *p_full  = sem_open(SEM_P_FULL,  0);
    sem_t *p_mutex = sem_open(SEM_P_MUTEX, 0);
    if (p_empty==SEM_FAILED || p_full==SEM_FAILED || p_mutex==SEM_FAILED) {
        perror("p1 sem_open pow"); exit(1);
    }

    // validación de p3 y p4 en ejecución
    int v1, v2, v3, w1, w2, w3;
    if (sem_getvalue(f_empty, &v1) || sem_getvalue(f_full, &v2) || sem_getvalue(f_mutex, &v3) ||
        sem_getvalue(p_empty, &w1) || sem_getvalue(p_full, &w2) || sem_getvalue(p_mutex, &w3)) {
        perror("p1 sem_getvalue"); exit(1);
    }

    // creación de p2
    pid_t pid = fork();
    if (pid < 0) { perror("p1 fork"); exit(1); }

    if (pid == 0) {
        // ejecución de potencias
        run_pow(a3, N, buf_p, p_empty, p_full, p_mutex);
        _exit(0);
    } else {
        //ejecución de fibonacci
        run_fibo(a1, a2, N, buf_f, f_empty, f_full, f_mutex);
        int st; waitpid(pid, &st, 0);
        // limpieza local
        munmap(buf_f, sizeof(shared_data)); close(shm_f);
        munmap(buf_p, sizeof(shared_data)); close(shm_p);
        sem_close(f_empty); sem_close(f_full); sem_close(f_mutex);
        sem_close(p_empty); sem_close(p_full); sem_close(p_mutex);
    }
    return 0;
}
