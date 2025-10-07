#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define CAPACIDAD 3
#define HOMBRE 1
#define MUJER 0

// Variables compartidas
int personas_en_banio = 0;
int banio_sexo = -1;
int hombres_dentro = 0, mujeres_dentro = 0;

// Mecanismos de sincronización
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_entrada = PTHREAD_COND_INITIALIZER;

void usar_banio(int id, int sexo)
{
    printf("%s %d entrando al baño. Hombres: %d, Mujeres: %d\n",
           sexo ? "Hombre" : "Mujer", id, hombres_dentro, mujeres_dentro);
    sleep(rand() % 2 + 1); // Simula uso del baño
    printf("%s %d saliendo del baño\n", sexo ? "Hombre" : "Mujer", id);
}

void *persona(void *arg)
{
    int id = *((int *)arg);
    int sexo = *((int *)arg + 1);

    pthread_mutex_lock(&mutex);

    // Esperar mientras:
    // 1. El baño está lleno O
    // 2. El baño es del otro género y no está vacío
    while (((hombres_dentro || mujeres_dentro) && banio_sexo != sexo) ||
           (banio_sexo == sexo &&
            ((sexo == HOMBRE && hombres_dentro >= CAPACIDAD) ||
             (sexo == MUJER && mujeres_dentro >= CAPACIDAD))))
    {
        printf("%s %d esperando... Baño actual: %s\n",
               sexo ? "Hombre" : "Mujer", id,
               banio_sexo == HOMBRE ? "Hombres" : (banio_sexo == MUJER ? "Mujeres" : "Vacío"));
        pthread_cond_wait(&cond_entrada, &mutex);
    }

    // Entrar al baño
    if (sexo)
        hombres_dentro++;
    else
        mujeres_dentro++;

    banio_sexo = sexo; // Actualizar género actual
    personas_en_banio++;

    pthread_mutex_unlock(&mutex);

    usar_banio(id, sexo);

    pthread_mutex_lock(&mutex);

    if (sexo)
        hombres_dentro--;
    else
        mujeres_dentro--;
    personas_en_banio--;

    // Notificar a todos cuando el baño se vacía
    if (!hombres_dentro && !mujeres_dentro)
    {
        banio_sexo = -1; // Baño vacío
        pthread_cond_broadcast(&cond_entrada);
    }

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main()
{
    srand(time(NULL)); // Inicializar semilla para números aleatorios

    pthread_t personas[10];
    int personas_data[10][2]; // [id, sexo]
    // Modificado el orden de los géneros para mostrar más interacciones
    bool generos[10] = {1, 0, 1, 0, 1, 0, 1, 0, 1, 0};

    printf("Simulación de baño unisex con capacidad %d\n", CAPACIDAD);
    printf("Secuencia de personas: H, M, H, M, H, M, H, M, H, M\n");

    for (int i = 0; i < 10; i++)
    {
        personas_data[i][0] = i + 1;
        personas_data[i][1] = generos[i];
        pthread_create(&personas[i], NULL, persona, &personas_data[i]);
        sleep(1); // Espacio entre llegada de personas
    }

    for (int i = 0; i < 10; i++)
    {
        pthread_join(personas[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_entrada);

    printf("Todas las personas han terminado de usar el baño.\n");
    return 0;
}