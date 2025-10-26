//puras pruebas

#include <stdio.h>
#include <unistd.h>
#include <unistd.h>

#include <fcntl.h>
#include <semaphore.h>
#include <string.h>

#include <errno.h> //manejo de errores

strtol()

memcpy()

int main(int argc, char *argv[]) {

    sem_getvalue();
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Error en fork(\n");
        return -1;
    }

    if (pid >0) {

        printf("Soy proceso padre con PID %d\n:", getpid())
        sem_wait(NULL);

    } else if (pid == 0){

        printf("Soy proceso hijo con PID %d y PPID %d\n", getpid(), getppid(  ))
    }

    // Comprobaciones
    if (argc != 5){
        perror("Uso: p1 N a1 a2 a3\n");
        exit(1)
    }

char *endtpr
errno = 0;

// verificar si los argv son dados correctamente, con ERRNO

int N = strtol(argv[1], &endptr, 10);

    if (errno != 0 || *endtpr != '\0') {

        perror("N debe ser un numero entero.\n");
        exit(1);

}

    int a1 = strtol(argv[2], &endptr, 10);

    if (errno != 0 || *endtpr != '\0') {

        perror("a1 debe ser un numero entero.\n");
        exit(1);

}

    int a2 = strtol(argv[3], &endptr, 10);

    if (errno != 0 || *endtpr != '\0') {

        perror("a2 debe ser un numero entero.\n");
        exit(1);

}

    int a3 = strtol(argv[4], &endptr, 10);

    if (errno != 0 || *endtpr != '\0') {

        perror("a3 debe ser un numero entero.\n");
        exit(1);

}

const char nombmemo[] = "/sharedMemory";

int fdm = shm_open(nombmemo, O_RDWR, 0666);

const char nombsem1[] = "/sem1";

sem_t *sem1 = sem_open(nombsem1, O_RDWR, 0666, 1);

if (sem1 == SEM_FAILED || sem2 == SEM_FAILED || sem3 == SEM_FAILED) {

    //cerrarlo de la memoria compartida nose

}



//STRLON

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s N a1 a2 a3\n", argv[0]);
        return 1;
    }

    char *endptr;
    long temp;
    int N, a1, a2, a3;

    // --- Validar N ---
    errno = 0;
    temp = strtol(argv[1], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || temp < 0 || temp > INT_MAX) {
        fprintf(stderr, "Error: N debe ser un entero válido.\n");
        return 1;
    }
    N = (int)temp;

    // --- Validar a1 ---
    errno = 0;
    temp = strtol(argv[2], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || temp > INT_MAX || temp < INT_MIN) {
        fprintf(stderr, "Error: a1 debe ser un entero válido.\n");
        return 1;
    }
    a1 = (int)temp;

    // --- Validar a2 ---
    errno = 0;
    temp = strtol(argv[3], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || temp > INT_MAX || temp < INT_MIN) {
        fprintf(stderr, "Error: a2 debe ser un entero válido.\n");
        return 1;
    }
    a2 = (int)temp;

    // --- Validar a3 ---
    errno = 0;
    temp = strtol(argv[4], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || temp > INT_MAX || temp < INT_MIN) {
        fprintf(stderr, "Error: a3 debe ser un entero válido.\n");
        return 1;
    }
    a3 = (int)temp;

    printf("Valores correctos: N=%d, a1=%d, a2=%d, a3=%d\n", N, a1, a2, a3);
    return 0;
}



if (argc != 5) {

        printf("Uso: %s N a1 a2 a3.\n", argv[0]);
        exit(1);

    }


// asdasd



int main(int argc, char *argv[]) {
    sem_t *sem_empty, *sem_full, *sem_mutex;
    int val_empty, val_full, val_mutex;

    //ntenta abrir los semáforos
    sem_empty = sem_open(SEM_EMPTY, 0);
    sem_full  = sem_open(SEM_FULL, 0);
    sem_mutex = sem_open(SEM_MUTEX, 0);

    if (sem_empty == SEM_FAILED || sem_full == SEM_FAILED || sem_mutex == SEM_FAILED) {
        perror("Error al abrir semáforos");
        fprintf(stderr, "P3 o P4 no están en ejecución.\n");
        exit(EXIT_FAILURE);
    }

    //verifica con getvalue
    if (sem_getvalue(sem_empty, &val_empty) == -1 ||
        sem_getvalue(sem_full, &val_full) == -1 ||
        sem_getvalue(sem_mutex, &val_mutex) == -1) {
        perror("sem_getvalue");
        fprintf(stderr, "P3 o P4 estén en ejecución.\n");
        exit(EXIT_FAILURE);
    }

    //si val es invalido, no estan bien inicializados
    if (val_empty < 0 || val_full < 0 || val_mutex < 0) {
        fprintf(stderr, "P3 o P4 no están correctamente inicializados.\n");
        exit(EXIT_FAILURE);
    }

    /* A partir de aquí, P1 puede crear a P2 */
    // fork() + exec() a P2, etc.

    return 0;
}
