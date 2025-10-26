#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <errno.h>

// Nombres típicos (ya los tienes definidos en tu código)
#define FIFO_P2 "/tmp/fifo_p2"

// Si decides usar turnos entre productores, define y crea/abre estos en P3 o en P1:
// #define SEM_TURN_P1 "/sem_turn_p1"
// #define SEM_TURN_P2 "/sem_turn_p2"

static inline int pot2(int exp) {
    // 2^exp sin usar pow(double) para evitar conversión/overflow raro
    // Asume exp >= 0 y resultado cabe en int (valídalo si hace falta)
    return (int)(1u << exp);
}

void p2_main(
    int N,
    int a3,
    volatile int *shm,          // puntero a la zona de memoria compartida (int)
    sem_t *sem_empty,
    sem_t *sem_full,
    sem_t *sem_mutex,
    // Los dos siguientes son OPCIONALES si implementas la regla de "máx 2 consecutivos"
    sem_t *sem_turn_p1,         // turno del productor P1 (Fibonacci)
    sem_t *sem_turn_p2,         // turno del productor P2 (potencias)
    int usar_turnos_productores // 0 = no usar, 1 = usar (máx 2 consecutivos)
) {
    // ---------- (opcional) esperar turno inicial de P2 ----------
    // Si decides que arranque P1, entonces aquí NO esperas y P1 hace wait(sem_turn_p1) al inicio.
    if (usar_turnos_productores) {
        // P2 esperará su turno para escribir (lo cede P1 con sem_post(sem_turn_p2))
        if (sem_turn_p2 == NULL || sem_turn_p1 == NULL) {
            fprintf(stderr, "Turnos de productores habilitados pero semáforos nulos.\n");
            exit(EXIT_FAILURE);
        }
    }

    int escritos = 0;
    while (escritos < N) {

        // ---------- (opcional) BLOQUE de máximo 2 consecutivos ----------
        if (usar_turnos_productores) {
            // Espera que P1 le ceda el turno (o que sea el inicio si decides que arranque P2)
            sem_wait(sem_turn_p2);
        }

        // Escribe hasta 2 valores (o los que falten) en rondas
        int quota = (N - escritos >= 2) ? 2 : (N - escritos);
        for (int k = 0; k < quota; ++k) {
            // Espera espacio en el buffer y entra en sección crítica
            sem_wait(sem_empty);
            sem_wait(sem_mutex);

            int valor = pot2(a3 + escritos);
            *((volatile int *)shm) = valor;   // escribir en el buffer de 1 entero
            printf("%d ", valor);              // salida estándar (P4 leerá y mostrará también)

            sem_post(sem_mutex);
            sem_post(sem_full);

            escritos++;
        }

        // Cede el turno a P1 para que no haya más de 2 consecutivos del mismo tipo
        if (usar_turnos_productores) {
            sem_post(sem_turn_p1);
        }
    }

    // ----------- enviar testigo de fin: -2 -----------
    sem_wait(sem_empty);
    sem_wait(sem_mutex);
    *((volatile int *)shm) = -2;
    printf("-2 ");                // opcional: mostrar que se escribió el testigo
    sem_post(sem_mutex);
    sem_post(sem_full);

    // ----------- esperar notificación de P4 por FIFO con -3 -----------
    int fd = open(FIFO_P2, O_RDONLY);
    if (fd == -1) {
        perror("open(FIFO_P2)");
        // Aun así intenta terminar limpiamente
    } else {
        int msg = 0;
        ssize_t n = read(fd, &msg, sizeof(msg));
        if (n == sizeof(msg) && msg == -3) {
            printf("-3 P2 termina\n");
        } else {
            // Si no llega -3, imprime final igualmente para no bloquear la práctica
            printf("P2 termina (no se recibió -3 por FIFO)\n");
        }
        close(fd);
    }
}
