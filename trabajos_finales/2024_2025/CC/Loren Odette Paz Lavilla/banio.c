#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define CAPACIDAD 4
#define HOMBRE 1
#define MUJER 0

// Variables compartidas
int banio_sexo = -1;
int hombres_dentro = 0, mujeres_dentro = 0;

// Mecanismos de sincronización
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_entrada = PTHREAD_COND_INITIALIZER;

// Función que simula el uso del baño
void usar_banio(int id, int sexo) {
    printf("%s %d está usando el baño\n", sexo ? "Hombre" : "Mujer", id);
    usleep(rand() % 2000000 + 1000000); // 1 a 3 segundos
    printf("%s %d terminó de usar el baño\n", sexo ? "Hombre" : "Mujer", id);
}

// Hilo de cada persona
void* persona(void* arg) {
    int id = *((int*)arg);
    int sexo = *((int*)arg + 1);

    pthread_mutex_lock(&mutex);

    // Esperar mientras el baño no está disponible para este sexo
    while (((hombres_dentro || mujeres_dentro) && banio_sexo != sexo) ||
           (banio_sexo == sexo &&
            ((sexo == HOMBRE && hombres_dentro >= CAPACIDAD) ||
             (sexo == MUJER && mujeres_dentro >= CAPACIDAD)))) {
        printf("%s %d esperando para entrar al baño...\n", sexo ? "Hombre" : "Mujer", id);
        pthread_cond_wait(&cond_entrada, &mutex);
    }

    // Entrar al baño
    if (sexo == HOMBRE) hombres_dentro++;
    else mujeres_dentro++;

    banio_sexo = sexo;

    pthread_mutex_unlock(&mutex);

    usar_banio(id, sexo);

    pthread_mutex_lock(&mutex);

    if (sexo == HOMBRE) hombres_dentro--;
    else mujeres_dentro--;

    // Si se vacía el baño, avisar a todos
    if (hombres_dentro == 0 && mujeres_dentro == 0) {
        pthread_cond_broadcast(&cond_entrada);
    }

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    srand(time(NULL));

    pthread_t personas[12];
    int personas_data[12][2];

    // Nueva secuencia de géneros: 7 mujeres, 5 hombres, distribuidos de forma diferente
    bool generos[12] = {0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0};

    // Crear hilos en orden inverso (del último al primero)
    for (int i = 11; i >= 0; i--) {
        personas_data[i][0] = i + 1;
        personas_data[i][1] = generos[i];
        pthread_create(&personas[i], NULL, persona, &personas_data[i]);
    }

    // Esperar a que terminen
    for (int i = 0; i < 12; i++) {
        pthread_join(personas[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_entrada);

    return 0;
}
