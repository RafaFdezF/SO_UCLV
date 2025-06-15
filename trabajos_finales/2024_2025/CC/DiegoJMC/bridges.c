#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_FARMERS 12 // Cambiado de 10 a 12
#define NORTH 0
#define SOUTH 1
#define MAX_CONSECUTIVE 2 // Cambiado de 3 a 2 (más restricción)

// Variables compartidas
int bridge_users = 0;
int current_direction = -1; // -1 = puente vacío
int north_count = 0;        // Contador de norte consecutivos
int south_count = 0;        // Contador de sur consecutivos

// Mecanismos de sincronización
pthread_mutex_t bridge_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t bridge_cond = PTHREAD_COND_INITIALIZER;

void cross_bridge(int direction, int farmer_id)
{
    printf("Agricultor %d cruzando hacia %s\n",
           farmer_id,
           (direction == NORTH) ? "NORTE" : "SUR");
    sleep(5); // Reducido de 10 a 5 segundos para hacer la simulación más rápida
}

void *farmer(void *arg)
{
    int direction = *((int *)arg);
    int farmer_id = *((int *)arg + 1);

    // ---- SECCIÓN CRÍTICA ----
    pthread_mutex_lock(&bridge_mutex);

    // Espera si:
    // 1. Hay agricultores en dirección contraria, O
    // 2. Ya se alcanzó MAX_CONSECUTIVE en su dirección.
    while ((bridge_users > 0 && current_direction != direction) ||
           (current_direction == direction &&
            ((direction == NORTH && north_count >= MAX_CONSECUTIVE) ||
             (direction == SOUTH && south_count >= MAX_CONSECUTIVE))))
    {
        pthread_cond_wait(&bridge_cond, &bridge_mutex);
    }

    // Actualiza estado del puente y contadores
    bridge_users++;
    current_direction = direction;
    if (direction == NORTH)
    {
        north_count++;
        south_count = 0; // Reinicia contador del sur
    }
    else
    {
        south_count++;
        north_count = 0; // Reinicia contador del norte
    }

    printf("Agricultor %d comenzando a cruzar hacia %s (Consecutivos: %d)\n",
           farmer_id,
           (direction == NORTH) ? "NORTE" : "SUR",
           (direction == NORTH) ? north_count : south_count);

    pthread_mutex_unlock(&bridge_mutex);
    // ---- FIN SECCIÓN CRÍTICA ----

    cross_bridge(direction, farmer_id);

    // ---- SECCIÓN CRÍTICA ----
    pthread_mutex_lock(&bridge_mutex);

    bridge_users--;

    printf("Agricultor %d terminó de cruzar hacia %s\n",
           farmer_id,
           (direction == NORTH) ? "NORTE" : "SUR");

    if (bridge_users == 0)
    { // ¡Solo notificar cuando pase el ultimo de su ronda!
        pthread_cond_broadcast(&bridge_cond);
    }

    pthread_mutex_unlock(&bridge_mutex);
    // ---- FIN SECCIÓN CRÍTICA ----

    return NULL;
}

int main()
{
    pthread_t farmers[MAX_FARMERS];
    int farmer_data[MAX_FARMERS][2]; // [direction, id]

    // Secuencia específica de entrada (NORTE=0, SUR=1)
    int order[MAX_FARMERS] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}; // Ejemplo ajustado

    // Crear hilos según la secuencia order[]
    for (int i = 0; i < MAX_FARMERS; i++)
    {
        farmer_data[i][0] = order[i];
        farmer_data[i][1] = i + 1;
        pthread_create(&farmers[i], NULL, farmer, &farmer_data[i]);
    }

    // Esperar a que todos terminen
    for (int i = 0; i < MAX_FARMERS; i++)
    {
        pthread_join(farmers[i], NULL);
    }

    // Limpieza
    pthread_mutex_destroy(&bridge_mutex);
    pthread_cond_destroy(&bridge_cond);

    printf("Todos los agricultores han cruzado\n");
    return 0;
}