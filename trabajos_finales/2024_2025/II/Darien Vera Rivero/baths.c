#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#define CAPACIDAD 5 // Capacidad modificada a 5
#define HOMBRE 1
#define MUJER 0

// Variables compartidas
int hombres_dentro = 0;
int mujeres_dentro = 0;
int banio_sexo = -1;

// Mecanismos de sincronización
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_entrada = PTHREAD_COND_INITIALIZER;

void usar_banio(int id, int sexo)
{
    printf("%s %d entrando al baño\n", sexo == HOMBRE ? "Hombre" : "Mujer", id);
    sleep(rand() % 2 + 1); // Simula uso del baño
    printf("%s %d saliendo del baño\n", sexo == HOMBRE ? "Hombre" : "Mujer", id);
}

void *persona(void *arg)
{
    int id = ((int *)arg)[0];
    int sexo = ((int *)arg)[1];

    pthread_mutex_lock(&mutex);

    // Esperar mientras:
    // 1. El baño está lleno para el mismo género
    // 2. El baño es del otro género y no está vacío
    while ((banio_sexo != -1 && banio_sexo != sexo) ||
           (sexo == HOMBRE && hombres_dentro >= CAPACIDAD) ||
           (sexo == MUJER && mujeres_dentro >= CAPACIDAD))
    {
        pthread_cond_wait(&cond_entrada, &mutex);
    }

    // Entrar al baño
    if (sexo == HOMBRE)
    {
        hombres_dentro++;
    }
    else
    {
        mujeres_dentro++;
    }
    banio_sexo = sexo;

    pthread_mutex_unlock(&mutex);

    usar_banio(id, sexo);

    pthread_mutex_lock(&mutex);

    // Salir del baño
    if (sexo == HOMBRE)
    {
        hombres_dentro--;
    }
    else
    {
        mujeres_dentro--;
    }

    // Si el baño queda vacío, permite cambiar de género
    if (hombres_dentro == 0 && mujeres_dentro == 0)
    {
        banio_sexo = -1;
        pthread_cond_broadcast(&cond_entrada);
    }
    else
    {
        // Si aún quedan personas del mismo género, señaliza a otro del mismo género
        pthread_cond_signal(&cond_entrada);
    }

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main()
{
    pthread_t personas[10];
    int personas_data[10][2];                          // [id, sexo]
    bool generos[10] = {0, 0, 1, 1, 0, 1, 0, 1, 0, 1}; // Orden de llegada modificado

    srand(time(NULL));
    for (int i = 0; i < 10; i++)
    {
        personas_data[i][0] = i + 1;
        personas_data[i][1] = generos[i] ? HOMBRE : MUJER;
        pthread_create(&personas[i], NULL, persona, &personas_data[i]);
    }

    for (int i = 0; i < 10; i++)
    {
        pthread_join(personas[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_entrada);

    return 0;
}
