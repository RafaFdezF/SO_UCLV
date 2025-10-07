#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#define CAPACIDAD 4
#define HOMBRE 1
#define MUJER 0

// Variables compartidas
int hombres_dentro = 0, mujeres_dentro = 0;
int banio_sexo = -1; // -1 significa vacío

// Mecanismos de sincronización
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_entrada = PTHREAD_COND_INITIALIZER;

void usar_banio(int id, int sexo)
{
    printf("%s %d entro al baño\n", sexo ? "Hombre" : "Mujer", id);
    sleep(rand() % 3 + 2); // Simula uso del baño (2-4 segundos)
    printf("%s %d salio del baño\n", sexo ? "Hombre" : "Mujer", id);
}

void *persona(void *arg)
{
    int id = ((int *)arg)[0];
    int sexo = ((int *)arg)[1];

    pthread_mutex_lock(&mutex);

    // Esperar mientras:
    // 1. El baño está ocupado por el otro género y no está vacío O
    // 2. El baño está lleno para mi género
    while ((banio_sexo != -1 && banio_sexo != sexo) || (sexo == HOMBRE && hombres_dentro >= CAPACIDAD) || (sexo == MUJER && mujeres_dentro >= CAPACIDAD))
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

    if (banio_sexo == -1)
    {
        banio_sexo = sexo; // Establecer género si está vacío
    }

    pthread_mutex_unlock(&mutex);

    usar_banio(id, sexo);

    pthread_mutex_lock(&mutex);

    if (sexo == HOMBRE)
    {
        hombres_dentro--;
    }
    else
    {
        mujeres_dentro--;
    }

    // Notificar a todos cuando el baño se vacía
    if (hombres_dentro == 0 && mujeres_dentro == 0)
    {
        banio_sexo = -1; // Marcar como vacío
        pthread_cond_broadcast(&cond_entrada);
    }
    else
    {
        pthread_cond_signal(&cond_entrada); // Notificar al menos a uno
    }

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main()
{

    pthread_t personas[14];
    int personas_data[14][2];                                      // [id, sexo]
    bool generos[14] = {1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1}; // 14 elementos

    for (int i = 0; i < 14; i++)
    {
        personas_data[i][0] = i + 1;      // ID
        personas_data[i][1] = generos[i]; // Sexo
        pthread_create(&personas[i], NULL, persona, personas_data[i]);
    }

    for (int i = 0; i < 14; i++)
    {
        pthread_join(personas[i], NULL);
    }

    printf("La cola del baño está vacía\n");
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_entrada);

    return 0;
}