#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

// MODIFICACIÓN 1: La capacidad del baño se cambió de 3 a 4.
#define CAPACIDAD 4
#define HOMBRE 1
#define MUJER 0

// Variables compartidas
int personas_en_banio = 0;
int banio_sexo = -1;
int hombres_dentro = 0, mujeres_dentro = 0;

// Mecanismos de sincronización
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_entrada = PTHREAD_COND_INITIALIZER;

void usar_banio(int id, int sexo) {
    printf("%s %d entrando al baño\n", sexo ? "Hombre" : "Mujer", id);
    sleep(rand() % 2 + 1); // Simula uso del baño
    printf("%s %d saliendo del baño\n", sexo ? "Hombre" : "Mujer", id);
}

void* persona(void* arg) {
    int id = *((int*)arg);
    int sexo = *((int*)arg + 1);

    pthread_mutex_lock(&mutex);

    // Esperar mientras:
    // 1. El baño está ocupado por el otro género.
    // 2. El baño es del mismo género pero ya está lleno.
    while (((hombres_dentro > 0 || mujeres_dentro > 0) && banio_sexo != sexo) ||
           (banio_sexo == sexo &&
            ((sexo == HOMBRE && hombres_dentro >= CAPACIDAD) ||
             (sexo == MUJER && mujeres_dentro >= CAPACIDAD)))) {
        pthread_cond_wait(&cond_entrada, &mutex);
    }

    // Entrar al baño
    if (sexo == HOMBRE) {
        hombres_dentro++;
    } else {
        mujeres_dentro++;
    }
    
    banio_sexo = sexo; // Establecer el género actual del baño

    pthread_mutex_unlock(&mutex);

    usar_banio(id, sexo);

    pthread_mutex_lock(&mutex);

    // Salir del baño
    if (sexo == HOMBRE) {
        hombres_dentro--;
    } else {
        mujeres_dentro--;
    }

    // Si el baño queda vacío, notificar a todos los hilos en espera.
    // Esto permite que los hilos del género opuesto puedan competir por entrar.
    if (hombres_dentro == 0 && mujeres_dentro == 0) {
        banio_sexo = -1; // Marcar el baño como vacío
        pthread_cond_broadcast(&cond_entrada);
    }

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    // MODIFICACIÓN 2: El número de personas (hilos) se cambió de 10 a 15.
    pthread_t personas[15];
    int personas_data[15][2];  // [id, sexo]

    // MODIFICACIÓN 3: Se cambió el orden y la cantidad de hombres/mujeres en la cola.
    // H: 1 (Hombre), M: 0 (Mujer)
    // Nueva cola: M, M, H, H, M, H, M, H, M, M, H, H, H, M, H
    bool generos[15] = {0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1};

    // Creación de los hilos
    for (int i = 0; i < 15; i++) {
        personas_data[i][0] = i + 1;
        personas_data[i][1] = generos[i];
        pthread_create(&personas[i], NULL, persona, &personas_data[i]);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < 15; i++) {
        pthread_join(personas[i], NULL);
    }

    // Liberar recursos
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_entrada);

    return 0;
}