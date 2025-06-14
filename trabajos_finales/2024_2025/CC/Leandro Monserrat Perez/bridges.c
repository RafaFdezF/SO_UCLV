#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_FARMERS       15
#define NORTH             0
#define SOUTH             1
#define MAX_CONSECUTIVE   5

// Variables compartidas
int bridge_users      = 0;
int current_direction = -1;    // -1 = puente vacío
int north_count       = 0;     // consecutivos norte
int south_count       = 0;     // consecutivos sur

pthread_mutex_t bridge_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  bridge_cond  = PTHREAD_COND_INITIALIZER;

void cross_bridge(int direction, int farmer_id) {
    printf("Agricultor %d cruzando hacia %s\n",
           farmer_id,
           (direction == NORTH) ? "NORTE" : "SUR");
    sleep(6);
}

void* farmer(void* arg) {
    int direction = ((int*)arg)[0];
    int farmer_id = ((int*)arg)[1];

    // ---- SECCIÓN CRÍTICA: espera para cruzar ----
    pthread_mutex_lock(&bridge_mutex);
    while (
        (bridge_users > 0 && current_direction != direction) ||
        (current_direction == direction &&
         ((direction == NORTH && north_count >= MAX_CONSECUTIVE) ||
          (direction == SOUTH && south_count >= MAX_CONSECUTIVE)))
    ) {
        pthread_cond_wait(&bridge_cond, &bridge_mutex);
    }

    // Actualiza estado antes de cruzar
    bridge_users++;
    current_direction = direction;
    if (direction == NORTH) {
        north_count++;
        south_count = 0;
    } else {
        south_count++;
        north_count = 0;
    }
    pthread_mutex_unlock(&bridge_mutex);
    // ---- FIN SECCIÓN CRÍTICA ----

    cross_bridge(direction, farmer_id);

    // ---- SECCIÓN CRÍTICA: al terminar cruce ----
    pthread_mutex_lock(&bridge_mutex);
    bridge_users--;
    if (bridge_users == 0) {
        current_direction = -1;               // ← reinicia el sentido
        pthread_cond_broadcast(&bridge_cond); // despierta a todos
    }
    pthread_mutex_unlock(&bridge_mutex);
    // ---- FIN SECCIÓN CRÍTICA ----

    return NULL;
}

int main() {
    pthread_t farmers[MAX_FARMERS];
    int farmer_data[MAX_FARMERS][2];
    int order[MAX_FARMERS] = {0,1,0,1,1,1,1,0,1,1,0,1,1,1,1};

    for (int i = 0; i < MAX_FARMERS; i++) {
        farmer_data[i][0] = order[i];
        farmer_data[i][1] = i + 1;
        pthread_create(&farmers[i], NULL, farmer, &farmer_data[i]);
    }
    for (int i = 0; i < MAX_FARMERS; i++) {
        pthread_join(farmers[i], NULL);
    }

    pthread_mutex_destroy(&bridge_mutex);
    pthread_cond_destroy(&bridge_cond);

    printf("Todos los agricultores han cruzado\n");
    return 0;
}
