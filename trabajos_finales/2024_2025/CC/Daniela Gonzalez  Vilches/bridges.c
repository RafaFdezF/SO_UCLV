#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_FARMERS 7// Reducido a 7 agricultores
#define NORTH 0
#define SOUTH 1
#define MAX_CONSECUTIVE 5// Se aumenta en 5 consecutivos en la misma dirección

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
    sleep(8); // Se reduce el tiempo de cruce a 8 segundos
}

void *farmer(void *arg)
{
    int direction = *((int *)arg);
    int farmer_id = *((int *)arg + 1);

    pthread_mutex_lock(&bridge_mutex);

    while ((bridge_users > 0 && current_direction != direction) ||
           (current_direction == direction &&
            ((direction == NORTH && north_count >= MAX_CONSECUTIVE) ||
             (direction == SOUTH && south_count >= MAX_CONSECUTIVE))))
    {
        pthread_cond_wait(&bridge_cond, &bridge_mutex);
    }

    bridge_users++;
    current_direction = direction;
    if (direction == NORTH)
    {
        north_count++;
        south_count = 0;
    }
    else
    {
        south_count++;
        north_count = 0;
    }

    pthread_mutex_unlock(&bridge_mutex);

    cross_bridge(direction, farmer_id);

    pthread_mutex_lock(&bridge_mutex);
    bridge_users--;
    if (bridge_users == 0)
    {
        pthread_cond_broadcast(&bridge_cond);
    }
    pthread_mutex_unlock(&bridge_mutex);

    return NULL;
}

int main()
{
    pthread_t farmers[MAX_FARMERS];
    int farmer_data[MAX_FARMERS][2]; // [direction, id]

    // Secuencia de entrada modificada (NORTE=0, SUR=1)
    int order[MAX_FARMERS] = {0, 1, 1, 1, 1, 1, 0}; 

    for (int i = 0; i < MAX_FARMERS; i++)
    {
        farmer_data[i][0] = order[i];
        farmer_data[i][1] = i + 1;
        pthread_create(&farmers[i], NULL, farmer, &farmer_data[i]);
    }

    for (int i = 0; i < MAX_FARMERS; i++)
    {
        pthread_join(farmers[i], NULL);
    }

    pthread_mutex_destroy(&bridge_mutex);
    pthread_cond_destroy(&bridge_cond);

    printf("Todos los agricultores han cruzado\n");
    return 0;
}