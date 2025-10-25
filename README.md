#  Entrega 1 – IPC y Sincronización de Procesos  
**Curso:** Sistemas Operativos
**Semestre:** 2025-02
**Universidad:** Universidad Nacional de Colombia
**Profesor:** Juan Felipe Muñoz Fernandez

## Autores

| Nombre completo | Correo institucional |
|------------------|----------------------|
| **Carlos Daniel Urresty Ascuntar** | curresty@unal.edu.co |
| **Felipe Aristizabal Giraldo** | faristizabalg@unal.edu.co |

---


## Descripción general

Este proyecto implementa la comunicación y sincronización entre **cuatro procesos (P1, P2, P3, P4)** en el sistema operativo **Linux**, utilizando los mecanismos **POSIX** de:

- Semáforos con nombre (`sem_open`, `sem_wait`, `sem_post`, `sem_unlink`, `sem_getvalue`)
- Memoria compartida (`shm_open`, `mmap`, `ftruncate`, `munmap`)
- Tuberías (`pipe`, `mkfifo`, `read`, `write`)

El objetivo es demostrar el control de concurrencia, sincronización y comunicación entre procesos que generan y consumen secuencias numéricas.  


## Estructura del proyecto

```
IPC-2025/
├── p1_p2.c      # Procesos P1 (Fibonacci) y P2 (Potencias de 2)
├── p3.c         # Proceso lector de la secuencia Fibonacci
├── p4.c         # Proceso lector de la secuencia de potencias de 2
├── Makefile     # Compilación automática
└── README.md    # Documento de presentación
```

---

## Descripción de procesos

| Proceso | Función principal | Comunicación |
|----------|------------------|---------------|
| **P1** | Genera `N` números de la secuencia de **Fibonacci**. | Escribe en memoria compartida. |
| **P2** | Genera `N` números de la secuencia de **potencias de 2**. | Escribe en memoria compartida. |
| **P3** | Lee y muestra los valores de **Fibonacci**. | Lee desde memoria compartida y usa semáforos. |
| **P4** | Lee y muestra las **potencias de 2**. | Lee desde memoria compartida y usa semáforos. |

Los procesos se sincronizan para evitar escrituras o lecturas consecutivas del mismo tipo.  
Cuando P1 y P2 finalizan, envían los testigos `-1` y `-2` respectivamente.  
P3 y P4 responden con `-3` mediante tuberías, indicando la finalización coordinada.  

---

## Ejecución

### Compilación
```bash
make
```

### Ejecución en tres terminales diferentes

**Terminal 1:**
```bash
./p3
```

**Terminal 2:**
```bash
./p4
```

**Terminal 3:**
```bash
./p1_p2 N a1 a2 a3
```

**Ejemplo:**
```bash
./p1_p2 5 0 1 2
```

### Limpieza
```bash
make clean
```

---

## Entregables

Se entregan **tres archivos `.c`** (p1_p2.c, p3.c, p4.c) a través de **UNVirtual** antes del **lunes 20 de octubre de 2025 a las 8:00 a.m.**  
Solo **una entrega por pareja**.  
El código debe estar correctamente indentado, comentado y sin líneas mayores a 80 caracteres.  

---
