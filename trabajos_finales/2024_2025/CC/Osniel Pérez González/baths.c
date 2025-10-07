#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#define CAPACIDAD 2  // Baño más pequeño
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
    sleep(rand() % 3 + 1); // Tiempo de uso aumentado (1-3 segundos)
    printf("%s %d saliendo del baño\n", sexo ? "Hombre" : "Mujer", id);
}

void* persona(void* arg) {
    int id = *((int*)arg);
    int sexo = *((int*)arg + 1);

    pthread_mutex_lock(&mutex);

    // Esperar mientras:
    // 1. El baño está lleno O
    // 2. El baño es del otro género y no está vacío
    while (((hombres_dentro || mujeres_dentro) && banio_sexo != sexo) ||
           (banio_sexo == sexo && 
            ((sexo == HOMBRE && hombres_dentro >= CAPACIDAD) || 
             (sexo == MUJER && mujeres_dentro >= CAPACIDAD)))) {
        printf("%s %d esperando...\n", sexo ? "Hombre" : "Mujer", id);
        pthread_cond_wait(&cond_entrada, &mutex);
    }

    // Entrar al baño
    if (sexo) hombres_dentro++;
    else mujeres_dentro++;

    banio_sexo = sexo;
    pthread_mutex_unlock(&mutex);

    usar_banio(id, sexo);

    pthread_mutex_lock(&mutex);
    if (sexo) hombres_dentro--;
    else mujeres_dentro--;

    // Notificar a todos cuando el baño se vacía
    if (!hombres_dentro && !mujeres_dentro) {
        pthread_cond_broadcast(&cond_entrada);
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    pthread_t personas[8];    // Número reducido de personas
    int personas_data[8][2];  // [id, sexo]
    // Secuencia alternada más marcada
    bool generos[8] = {1, 1, 0, 0, 1, 1, 0, 0}; 

    srand(time(NULL)); // Mejor semilla para aleatoriedad

    for (int i = 0; i < 8; i++) {
        personas_data[i][0] = i + 1;
        personas_data[i][1] = generos[i];
        pthread_create(&personas[i], NULL, persona, &personas_data[i]);
        usleep(200000); // Pequeño retardo entre creación de hilos
    }

    for (int i = 0; i < 8; i++) {
        pthread_join(personas[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_entrada);

    return 0;
}