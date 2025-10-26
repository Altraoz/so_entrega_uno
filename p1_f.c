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

void fibonacci(int a1, int a2, int n, shared_data *data,
               sem_t *sem_1, sem_t *sem_2, sem_t *sem_3) {

    for (int i = 0; i < n; i++) {

        int val;
        if (i == 0) val = a1;
        else if (i == 1) val = a2;
        else {

            val = a1 + a2;
            a1 = a2;
            a2 = val;

        }

        sem_wait(sem_1);
        sem_wait(sem_3);

        data->value = val;

        sem_post(sem_3);
        sem_post(sem_2);
    }

    sem_wait(sem_1);
    sem_wait(sem_3);
    data->value = -1;
    sem_post(sem_3);
    sem_post(sem_2);
}

int main(int argc, char *argv[]) {

    if (argc != 5) {
        
        fprintf(stderr, "Uso: %s N a1 a2 a3\n", argv[0]);
        return 1;

    }

    char *endptr;
    long temp;
    int N, a1, a2, a3;

    //Error N. temp <x, >x, verifica si ingresaron un numero muy grande/pequeno
    errno = 0;
    temp = strtol(argv[1], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || temp < 0 || temp > INT_MAX) {

        fprintf(stderr, "N debe ser un entero.\n");
        return 1;

    }
    N = (int)temp;

    //Error a1
    errno = 0;
    temp = strtol(argv[2], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || temp > INT_MAX || temp < INT_MIN) {

        fprintf(stderr, "a1 debe ser un entero.\n");
        return 1;

    }
    a1 = (int)temp;

    //Error a2
    errno = 0;
    temp = strtol(argv[3], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || temp > INT_MAX || temp < INT_MIN) {

        fprintf(stderr, "a2 debe ser un entero.\n");
        return 1;

    }
    a2 = (int)temp;

    //Error a3
    errno = 0;
    temp = strtol(argv[4], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || temp > INT_MAX || temp < INT_MIN) {

        fprintf(stderr, "a3 debe ser un entero.\n");
        return 1;

    }
    a3 = (int)temp;

    //int N = atoi(argv[1]); estos no detectan errores
    //int a1 = atoi(argv[2]);
    //int a2 = atoi(argv[3]);
    //int a3 = atoi(argv[4]);

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

        execl("./p2", "p2", argv[1], argv[4], (char *)NULL);
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