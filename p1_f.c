//este es el "bueno"
//assadasd
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

#define SHM_NOM "/mem_compartida"
#define SEM_EMPTY "/sem_1" // /sem_empty
#define SEM_FULL "/sem_2"   // /sem_full
#define SEM_MUTEX "/sem_3" // /sem_mutex

typedef struct {

    int value;
} shared_data;

#include <sys/wait.h>

int parse_int(const char *arg, const char *nombre, long min, long max){
    char *endptr;
    errno = 0;
    long temp = strtol(arg, &endptr, 10);
    
    // validaciones de conversión ;)
    if (errno != 0 || *endptr != '\0' || temp < min || temp > max){
        fprintf(stderr, "%s debe ser un entero válido entre %ld y %ld.\n", nombre, min, max);
        exit(EXIT_FAILURE);
    }
}

void fibonacci(int a1, int a2, int N, shared_data *data,
               sem_t *sem_1, sem_t *sem_2, sem_t *sem_3) {

    for (int i = 0; i < N; i++) {
        int next = a1 + a2;
        a1 = a2;
        a2 = next;

        sem_wait(sem_1);
        sem_wait(sem_3);

        data->value = a1; // or next, depending on how you want to display it

        sem_post(sem_3);
        sem_post(sem_2);
    }

    sem_wait(sem_1);
    sem_wait(sem_3);
    data->value = -1;
    sem_post(sem_3);
    sem_post(sem_2);

    // Esperar confirmación de P4 mediante FIFO
    int fd = open("/tmp/fifo_p1", O_RDONLY);
    int msg;
    read(fd, &msg, sizeof(msg));

    if (msg == -3) {
        printf("-3 p1 termina\n");
    }
    close(fd);
}

void power_of_two(int a3, int n, shared_data *data,
                  sem_t *sem_1, sem_t *sem_2, sem_t *sem_3) {

    for (int i = 0; i < n; i++) {

        int val = 1 << (a3 + i); // 2^(a3 + i)

        sem_wait(sem_1); // se bloquea si es 0, entra y decrementa a 0
        sem_wait(sem_3); // 

        data->value = val;

        sem_post(sem_3);
        sem_post(sem_2);
    }

    sem_wait(sem_1);
    sem_wait(sem_3);
    data->value = -2;
    sem_post(sem_3);
    sem_post(sem_2);

    // Esperar confirmación de P4 mediante FIFO
    int fd = open("/tmp/fifo_p2", O_RDONLY);
    int msg;
    read(fd, &msg, sizeof(msg));

    if (msg == -3) {
        printf("-3 P2 termina\n");
    }
    close(fd);
}


int main(int argc, char *argv[]) {

    if (argc != 5) {
        
        fprintf(stderr, "Uso: %s N a1 a2 a3\n", argv[0]);
        return 1;

    }

    char *endptr;
    long temp;
    int N, a1, a2, a3;
    N = parse_int(argv[1], "N", 0, INT_MAX);
    a1 = parse_int(argv[2], "a1", INT_MIN, INT_MAX);
    a2 = parse_int(argv[3], "a2", INT_MIN, INT_MAX);
    a3 = parse_int(argv[4], "a3", INT_MIN, INT_MAX);

    sem_t *sem_1, *sem_2, *sem_3;
    int val_1, val_2, val_3; // val_empty, full, y mutex como los sem_

    //intenta abrir los semáforos
    sem_1 = sem_open(SEM_EMPTY, 0);
    sem_2  = sem_open(SEM_FULL, 0);
    sem_3 = sem_open(SEM_MUTEX, 0);

    if (sem_1 == SEM_FAILED || sem_2 == SEM_FAILED || sem_3 == SEM_FAILED) {

        perror("Error al abrir semáforos");
        fprintf(stderr, "P3 o P4 no están en ejecución.\n");
        exit(EXIT_FAILURE);

    }

    //verifica con getvalue
    if (sem_getvalue(sem_1, &val_1) == -1 ||
        sem_getvalue(sem_2, &val_2) == -1 ||
        sem_getvalue(sem_3, &val_3) == -1) {

        perror("Error sem_getvalue");
        fprintf(stderr, "P3 o P4 estén en ejecución.\n");
        exit(EXIT_FAILURE);

    }

    //si val es invalido, no estan bien inicializados
    if (val_1 < 0 || val_2 < 0 || val_3 < 0) {

        fprintf(stderr, "P3 o P4 no están correctamente inicializados.\n");
        exit(EXIT_FAILURE);

    }

    //inicializar shm_fd como mem

    int mem;     
    mem = shm_open(SHM_NOM, O_CREAT | O_RDWR, 0666);
    if (mem == -1) {

        perror("Error shm_open");
        return 1;

    }

    shared_data *data = mmap(NULL, sizeof(shared_data),
                              PROT_READ | PROT_WRITE, MAP_SHARED, mem, 0);
    if (data == MAP_FAILED) { 

        perror("Error mmap"); 
        exit(1); 

    }
    
    pid_t pid = fork();

    if (pid < 0) {

        perror("Error en fork(\n");
        return -1;

     }

    if (pid == 0) {
        power_of_two(a3, N, data, sem_1, sem_2, sem_3);
        perror("execl");
        exit(1);

    } else {

        fibonacci(a1, a2, N, data, sem_1, sem_2, sem_3);
        int status;
        waitpid(pid, &status, 0);
        printf("-3 P1 termina\n");

    }

    munmap(data, sizeof(shared_data));
    close(mem);
    sem_close(sem_1);
    sem_close(sem_2);
    sem_close(sem_3);

    return 0;
}