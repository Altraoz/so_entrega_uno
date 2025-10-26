//este es el "bueno"

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

#define SHM_NAME "/buffer_compartido"
#define SEM_EMPTY "/sem_empty"
#define SEM_FULL "/sem_full"
#define SEM_MUTEX "/sem_mutex"

typedef struct {

    int value;

} shared_data;

#include <sys/wait.h>

int parse_int(const char *arg, const char *nombre, long min, long max){
    char *endptr;
    errno = 0
    long temp = strtol(arg, &endptr, 10);
    
    // validaciones de conversión ;)
    if (errno != 0 || *endptr != '\0' || temp < min || temp > max){
        fprintf(stderr, "%s debe ser un entero válido entre %ld y %ld.\n", nombre, min, max);
        exit(EXIT_FAILURE);
    }
}



void fibonacci(int a1, int a2, int n, shared_data *data,
               sem_t *sem_empty, sem_t *sem_full, sem_t *sem_mutex) {

    for (int i = 0; i < n; i++) {

        int val;
        if (i == 0) val = a1;
        else if (i == 1) val = a2;
        else {

            val = a1 + a2;
            a1 = a2;
            a2 = val;

        }

        sem_wait(sem_empty);
        sem_wait(sem_mutex);

        data->value = val;

        sem_post(sem_mutex);
        sem_post(sem_full);
    }

    sem_wait(sem_empty);
    sem_wait(sem_mutex);
    data->value = -1;
    sem_post(sem_mutex);
    sem_post(sem_full);
}

void power_of_two(int a3, int n, shared_data *data,
                  sem_t *sem_empty, sem_t *sem_full, sem_t *sem_mutex) {

    for (int i = 0; i < n; i++) {

        int val = 1 << (a3 + i); // 2^(a3 + i)

        sem_wait(sem_empty); // se bloquea si es 0, entra y decrementa a 0
        sem_wait(sem_mutex); // 

        data->value = val;

        sem_post(sem_mutex);
        sem_post(sem_full);
    }

    sem_wait(sem_empty);
    sem_wait(sem_mutex);
    data->value = -2;
    sem_post(sem_mutex);
    sem_post(sem_full);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s N a1 a2 a3\n", argv[0]);
        return 1;
    }

    char *endptr;
    long temp;
    int N, a1, a2, a3;

    int N = parse_int(argv[1], "N", 0, INT_MAX);
    int a1 = parse_int(argv[2], "a1", INT_MIN, INT_MAX);
    int a2 = parse_int(argv[3], "a2", INT_MIN, INT_MAX);
    int a3 = parse_int(argv[4], "a3", INT_MIN, INT_MAX);

    // //Error N
    // errno = 0;
    // temp = strtol(argv[1], &endptr, 10);
    // if (errno != 0 || *endptr != '\0' || temp < 0 || temp > INT_MAX) {
    //     fprintf(stderr, "N debe ser un entero.\n");
    //     return 1;
    // }
    // N = (int)temp;

    // //Error a1
    // errno = 0;
    // temp = strtol(argv[2], &endptr, 10);
    // if (errno != 0 || *endptr != '\0' || temp > INT_MAX || temp < INT_MIN) {
    //     fprintf(stderr, "a1 debe ser un entero.\n");
    //     return 1;
    // }
    // a1 = (int)temp;

    // //Error a2
    // errno = 0;
    // temp = strtol(argv[3], &endptr, 10);
    // if (errno != 0 || *endptr != '\0' || temp > INT_MAX || temp < INT_MIN) {
    //     fprintf(stderr, "a2 debe ser un entero.\n");
    //     return 1;
    // }
    // a2 = (int)temp;

    // //Error a3
    // errno = 0;
    // temp = strtol(argv[4], &endptr, 10);
    // if (errno != 0 || *endptr != '\0' || temp > INT_MAX || temp < INT_MIN) {
    //     fprintf(stderr, "a3 debe ser un entero.\n");
    //     return 1;
    // }
    // a3 = (int)temp;

    //int N = atoi(argv[1]); estos no detectan errores
    //int a1 = atoi(argv[2]);
    //int a2 = atoi(argv[3]);
    //int a3 = atoi(argv[4]);

    sem_t *sem_empty = sem_open(SEM_EMPTY, 0);
    if (sem_empty == SEM_FAILED) {

        printf("P3 o P4 no están en ejecución.\n");
        exit(1);

    }

    sem_t *sem_full  = sem_open(SEM_FULL, 0);
    sem_t *sem_mutex = sem_open(SEM_MUTEX, 0);

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);

    if (shm_fd == -1) { 

        perror("shm_open"); exit(1); 

    }

    shared_data *data = mmap(NULL, sizeof(shared_data),
                              PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) { 
        perror("mmap"); exit(1); 
    }

//if (sem1 == SEM_FAILED || sem2 == SEM_FAILED || sem3 == SEM_FAILED) {cerrarlo de la memoria compartida nose}
    
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error en fork(\n");
        return -1;
     }
    if (pid == 0) {
        power_of_two(a3, N, data, sem_empty, sem_full, sem_mutex);
        perror("execl");
        exit(1);
    } else {
        fibonacci(a1, a2, N, data, sem_empty, sem_full, sem_mutex);
        int status;
        waitpid(pid, &status, 0);
        printf("-3 P1 termina\n");

    }

    munmap(data, sizeof(shared_data));
    close(shm_fd);
    sem_close(sem_empty);
    sem_close(sem_full);
    sem_close(sem_mutex);

    return 0;
}
