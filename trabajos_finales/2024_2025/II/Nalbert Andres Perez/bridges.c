#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#define NORTH 0
#define SOUTH 1

// Variables configurables mediante CLI
int max_farmers = 10;
int max_consecutive = 3;
int random_order = 0;  // 0 para orden fijo, 1 para aleatorio

// Variables compartidas
int bridge_users = 0;
int current_direction = -1;  // -1 = puente vacío
int north_count = 0;         // Contador de norte consecutivos
int south_count = 0;         // Contador de sur consecutivos

// Mecanismos de sincronización
pthread_mutex_t bridge_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  bridge_cond  = PTHREAD_COND_INITIALIZER;

void cross_bridge(int direction, int farmer_id) {
    printf("Agricultor %d cruzando hacia %s\n", 
           farmer_id, 
           (direction == NORTH) ? "NORTE" : "SUR");
    sleep(1);  // Reducido para pruebas más rápidas
}

void* farmer(void* arg) {
    int direction = *((int*)arg);
    int farmer_id = *((int*)arg + 1);

    // ---- SECCIÓN CRÍTICA ----
    pthread_mutex_lock(&bridge_mutex);

    // Espera si:
    // 1. Hay agricultores en dirección contraria, O
    // 2. Ya se alcanzó MAX_CONSECUTIVE en su dirección.
    while ((bridge_users > 0 && current_direction != direction) ||
           (current_direction == direction && 
            ((direction == NORTH && north_count >= max_consecutive) || 
             (direction == SOUTH && south_count >= max_consecutive)))) {
        pthread_cond_wait(&bridge_cond, &bridge_mutex);
    }

    // Actualiza estado del puente y contadores
    bridge_users++;
    current_direction = direction;
    if (direction == NORTH) {
        north_count++;
        south_count = 0;    // Reinicia contador del sur
    } else {
        south_count++;
        north_count = 0;    // Reinicia contador del norte
    }

    printf("Agricultor %d empieza a cruzar hacia %s (N:%d, S:%d)\n", 
           farmer_id, 
           (direction == NORTH) ? "NORTE" : "SUR",
           north_count, south_count);

    pthread_mutex_unlock(&bridge_mutex);
    // ---- FIN SECCIÓN CRÍTICA ----

    cross_bridge(direction, farmer_id);

    // ---- SECCIÓN CRÍTICA ----
    pthread_mutex_lock(&bridge_mutex);

    bridge_users--;

    printf("Agricultor %d terminó de cruzar hacia %s\n", 
           farmer_id, 
           (direction == NORTH) ? "NORTE" : "SUR");

    if (bridge_users == 0) {  // ¡Solo notificar cuando pase el ultimo de su ronda!
        pthread_cond_broadcast(&bridge_cond);
    }

    pthread_mutex_unlock(&bridge_mutex);
    // ---- FIN SECCIÓN CRÍTICA ----

    return NULL;
}

int main(int argc, char *argv[]) {
    // Configuración desde CLI
    if (argc > 1) max_farmers = atoi(argv[1]);
    if (argc > 2) max_consecutive = atoi(argv[2]);
    if (argc > 3) random_order = atoi(argv[3]);

    pthread_t farmers[max_farmers];
    int farmer_data[max_farmers][2];  // [direction, id]

    // Orden de entrada
    int order[max_farmers];
    
    if (random_order) {
        srand(time(NULL));
        for (int i = 0; i < max_farmers; i++) {
            order[i] = rand() % 2;  // 0 o 1 aleatorio
        }
    } else {
        // Orden fijo de ejemplo (alternado modificado)
        int fixed_order[] = {0, 1, 1, 1, 1, 0, 0, 1, 0, 1};
        for (int i = 0; i < max_farmers; i++) {
            order[i] = fixed_order[i % 10];
        }
    }

    printf("Simulación con %d agricultores, máximo %d consecutivos, orden %s\n",
           max_farmers, max_consecutive, random_order ? "aleatorio" : "fijo");
    printf("Secuencia de direcciones: ");
    for (int i = 0; i < max_farmers; i++) {
        printf("%s ", order[i] == NORTH ? "N" : "S");
    }
    printf("\n\n");

    // Crear hilos según la secuencia order[]
    for (int i = 0; i < max_farmers; i++) {
        farmer_data[i][0] = order[i];
        farmer_data[i][1] = i + 1;
        pthread_create(&farmers[i], NULL, farmer, &farmer_data[i]);
    }

    // Esperar a que todos terminen
    for (int i = 0; i < max_farmers; i++) {
        pthread_join(farmers[i], NULL);
    }

    // Limpieza
    pthread_mutex_destroy(&bridge_mutex);
    pthread_cond_destroy(&bridge_cond);

    printf("\nTodos los agricultores han cruzado\n");
    return 0;
}