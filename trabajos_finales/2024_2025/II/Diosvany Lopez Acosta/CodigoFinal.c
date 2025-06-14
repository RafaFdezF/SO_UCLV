#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define CAPACIDAD 2  // Nuevo tamaño reducido del baño
#define HOMBRE 1
#define MUJER 0

// Variables compartidas
int banio_sexo = -1;
int hombres_dentro = 0, mujeres_dentro = 0;
int siguiente_turno = 0;  // Para implementar FIFO
int turno_actual = 0;     // Para implementar FIFO

// Mecanismos de sincronización
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_entrada = PTHREAD_COND_INITIALIZER;

void usar_banio(int id, int sexo) {
    printf("%s %d entrando al baño\n", sexo ? "Hombre" : "Mujer", id);
    sleep(rand() % 2 + 1); // Simula uso del baño
    printf("%s %d saliendo del baño\n", sexo ? "Hombre" : "Mujer", id);
}

void* persona(void* arg) {
    int id = *((int*)arg);
    int sexo = *((int*)arg + 1);
    int mi_turno;

    pthread_mutex_lock(&mutex);
    
    // Tomar un número de turno
    mi_turno = siguiente_turno++;
    
    // Esperar mientras:
    // 1. No es mi turno O
    // 2. El baño está lleno O
    // 3. El baño es del otro género y no está vacío
    while (mi_turno != turno_actual || 
        ((hombres_dentro || mujeres_dentro) && banio_sexo != sexo) ||
        (banio_sexo == sexo && 
            ((sexo == HOMBRE && hombres_dentro >= CAPACIDAD) || 
            (sexo == MUJER && mujeres_dentro >= CAPACIDAD)))) {
        pthread_cond_wait(&cond_entrada, &mutex);
    }

    // Entrar al baño
    if (sexo) hombres_dentro++;
    else mujeres_dentro++;
    
    banio_sexo = sexo; // Actualizar género actual
    turno_actual++;     // Avanzar el turno actual
    
    pthread_mutex_unlock(&mutex);

    usar_banio(id, sexo);

    pthread_mutex_lock(&mutex);

    if (sexo) hombres_dentro--;
    else mujeres_dentro--;

    // Notificar a todos cuando el baño se vacía o cuando hay espacio
    if ((!hombres_dentro && !mujeres_dentro) || 
        (banio_sexo == HOMBRE && hombres_dentro < CAPACIDAD) || 
        (banio_sexo == MUJER && mujeres_dentro < CAPACIDAD)) {
        pthread_cond_broadcast(&cond_entrada);
    }

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    srand(time(NULL));
    pthread_t personas[10];    
    int personas_data[10][2];  // [id, sexo]
    bool generos[10] = {1, 0, 1, 1, 1, 0, 1, 0, 1, 1}; // Orden original

    printf("Capacidad del baño: %d personas\n", CAPACIDAD);
    printf("Orden de llegada: ");
    for (int i = 0; i < 10; i++) {
        printf("%s%d ", generos[i] ? "H" : "M", i+1);
    }
    printf("\n\n");

    for (int i = 0; i < 10; i++) {
        personas_data[i][0] = i + 1;
        personas_data[i][1] = generos[i];
        pthread_create(&personas[i], NULL, persona, &personas_data[i]);
    }

    for (int i = 0; i < 10; i++) {
        pthread_join(personas[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_entrada);

    return 0;
}