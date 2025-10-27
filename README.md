#  Entrega 1 – IPC y Sincronización de Procesos  
**Curso:** Sistemas Operativos
**Semestre:** 2025-02
**Universidad:** Universidad Nacional de Colombia
**Profesor:** Juan Felipe Muñoz Fernandez

## Autores

| Nombre completo | Correo institucional | Cédula |
|------------------|----------------------|----------|
| **Carlos Daniel Urresty Ascuntar** | curresty@unal.edu.co | 1006999238 |
| **Felipe Aristizabal Giraldo** | faristizabalg@unal.edu.co | 1013458353 |

---


## Descripción general

Este proyecto implementa la comunicación y sincronización entre **cuatro procesos (P1, P2, P3, P4)** en el sistema operativo **Linux**, utilizando los mecanismos **POSIX** de:

- Semáforos con nombre (`sem_open`, `sem_wait`, `sem_post`, `sem_unlink`, `sem_getvalue`)
- Memoria compartida (`shm_open`, `mmap`, `ftruncate`, `munmap`)
- Tuberías (`pipe`, `mkfifo`, `read`, `write`)

El objetivo es demostrar el control de concurrencia, sincronización y comunicación entre procesos que generan y consumen secuencias numéricas.  

