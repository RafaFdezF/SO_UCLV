#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#define CAPACIDAD 5
#define HOMBRE 1
#define MUJER 0
#define MAX_ESPERA 3
#define TOTAL_PERSONAS 15

int personas_en_banio = 0;
int banio_sexo = -1;
int hombres_dentro = 0, mujeres_dentro = 0;
int mujeres_esperando = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_entrada = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_mujeres = PTHREAD_COND_INITIALIZER;

void usar_banio(int id, int sexo) {
    printf("%s %d entrando al baño\n", sexo ? "Hombre" : "Mujer", id);
    sleep(rand() % 2 + 1);
    printf("%s %d saliendo del baño\n", sexo ? "Hombre" : "Mujer", id);
}

void* persona(void* arg) {
    int id = *((int*)arg);
    int sexo = *((int*)arg + 1);
    struct timespec tiempo_espera;
    int espera_expirada = 0;

    pthread_mutex_lock(&mutex);

    if (sexo == MUJER) {
        mujeres_esperando++;
    }

    clock_gettime(CLOCK_REALTIME, &tiempo_espera);
    tiempo_espera.tv_sec += MAX_ESPERA;

    while (((hombres_dentro || mujeres_dentro) && banio_sexo != sexo) ||
           (banio_sexo == sexo &&
            ((sexo == HOMBRE && hombres_dentro >= CAPACIDAD) ||
             (sexo == MUJER && mujeres_dentro >= CAPACIDAD))) ||
           (sexo == HOMBRE && mujeres_esperando > 0)) {
        
        if (sexo == HOMBRE) {
            espera_expirada = pthread_cond_timedwait(&cond_entrada, &mutex, &tiempo_espera);
            if (espera_expirada == ETIMEDOUT) {
                printf("Hombre %d se fue por tiempo de espera\n", id);
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
        } else {
            pthread_cond_wait(&cond_mujeres, &mutex);
        }
    }

    if (sexo) {
        hombres_dentro++;
    } else {
        mujeres_dentro++;
        mujeres_esperando--;
    }

    banio_sexo = sexo;

    pthread_mutex_unlock(&mutex);

    usar_banio(id, sexo);

    pthread_mutex_lock(&mutex);

    if (sexo) {
        hombres_dentro--;
    } else {
        mujeres_dentro--;
    }

    if (!hombres_dentro && !mujeres_dentro) {
        if (mujeres_esperando > 0) {
            pthread_cond_broadcast(&cond_mujeres);
        } else {
            pthread_cond_broadcast(&cond_entrada);
        }
    }

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    srand(time(NULL));
    pthread_t personas[TOTAL_PERSONAS];
    int personas_data[TOTAL_PERSONAS][2];
    bool generos[TOTAL_PERSONAS] = {1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0};

    printf("\033[1;32m");
    printf("Alexis González Rodríguez@SistemasOperativos:~$ ");
    printf("\033[0m");
    printf("Iniciando simulación del baño mixto\n\n");

    printf("Capacidad del baño: %d personas\n", CAPACIDAD);
    printf("Tiempo máximo de espera: %d segundos\n", MAX_ESPERA);
    printf("Total de personas: %d\n\n", TOTAL_PERSONAS);

    for (int i = 0; i < TOTAL_PERSONAS; i++) {
        personas_data[i][0] = i + 1;
        personas_data[i][1] = generos[i];
        pthread_create(&personas[i], NULL, persona, &personas_data[i]);
        usleep(100000);
    }

    for (int i = 0; i < TOTAL_PERSONAS; i++) {
        pthread_join(personas[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_entrada);
    pthread_cond_destroy(&cond_mujeres);

    printf("\033[1;32m");
    printf("Alexis González Rodríguez@SistemasOperativos:~$ ");
    printf("\033[0m");
    printf("Simulación finalizada\n\n");
    return 0;
} 