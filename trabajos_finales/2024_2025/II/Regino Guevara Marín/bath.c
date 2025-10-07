#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>  // [NUEVO] Para funciones de tiempo y semilla aleatoria

#define CAPACIDAD 5
#define HOMBRE 1
#define MUJER 0
#define MAX_PERSONAS 10  // [NUEVO] Límite máximo de personas en la simulación

// Variables compartidas
int personas_en_banio = 0;
int banio_sexo = -1;  // -1: vacío, 0: mujeres, 1: hombres
int hombres_dentro = 0, mujeres_dentro = 0;
int hombres_esperando = 0, mujeres_esperando = 0;  // [NUEVO] Contadores de espera

// Mecanismos de sincronización
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_hombres = PTHREAD_COND_INITIALIZER;  // [NUEVO] Condición separada para hombres
pthread_cond_t cond_mujeres = PTHREAD_COND_INITIALIZER;  // [NUEVO] Condición separada para mujeres

void usar_banio(int id, int sexo) {
    // [MODIFICADO] Mensajes más descriptivos y tiempo aleatorio más variado
    printf("%s %d usando el baño...\n", sexo ? "Hombre" : "Mujer", id);
    sleep((rand() % 3) + 1);  // Tiempo aleatorio entre 1-4 segundos (antes era 1-2)
    printf("%s %d terminó de usar el baño\n", sexo ? "Hombre" : "Mujer", id);
}

void* persona(void* arg) {
    int id = *((int*)arg);
    int sexo = *((int*)arg + 1);
    
    // [NUEVO] Sección para registrar la llegada a la fila
    pthread_mutex_lock(&mutex);
    if (sexo == HOMBRE) {
        hombres_esperando++;
        printf("Hombre %d llegó a la fila (Esperando: %dH %dM)\n", 
               id, hombres_esperando, mujeres_esperando);
    } else {
        mujeres_esperando++;
        printf("Mujer %d llegó a la fila (Esperando: %dH %dM)\n", 
               id, hombres_esperando, mujeres_esperando);
    }
    pthread_mutex_unlock(&mutex);
    
    // Intentar entrar al baño
    pthread_mutex_lock(&mutex);
    
    // [MODIFICADO] Condiciones de espera mejoradas:
    // 1. Baño lleno
    // 2. Baño ocupado por el otro sexo
    while ((personas_en_banio >= CAPACIDAD) ||
           ((banio_sexo != -1) && (banio_sexo != sexo))) {
        // [NUEVO] Usamos variables de condición separadas por sexo
        if (sexo == HOMBRE) {
            pthread_cond_wait(&cond_hombres, &mutex);
        } else {
            pthread_cond_wait(&cond_mujeres, &mutex);
        }
    }
    
    // Entrar al baño
    if (sexo == HOMBRE) {
        hombres_esperando--;  // [NUEVO] Actualizar contador de espera
        hombres_dentro++;
    } else {
        mujeres_esperando--;  // [NUEVO] Actualizar contador de espera
        mujeres_dentro++;
    }
    
    personas_en_banio++;
    banio_sexo = sexo;
    
    // [MODIFICADO] Mensaje más informativo
    printf("%s %d entró al baño (Dentro: %dH %dM)\n", 
           sexo ? "Hombre" : "Mujer", id, hombres_dentro, mujeres_dentro);
    
    pthread_mutex_unlock(&mutex);
    
    // Usar el baño
    usar_banio(id, sexo);
    
    // Salir del baño
    pthread_mutex_lock(&mutex);
    
    if (sexo == HOMBRE) {
        hombres_dentro--;
    } else {
        mujeres_dentro--;
    }
    
    personas_en_banio--;
    
    // [MODIFICADO] Mensaje más informativo
    printf("%s %d salió del baño (Dentro: %dH %dM)\n", 
           sexo ? "Hombre" : "Mujer", id, hombres_dentro, mujeres_dentro);
    
    // [MODIFICADO] Lógica mejorada para vaciar el baño
    if (personas_en_banio == 0) {
        banio_sexo = -1; // Baño vacío
        
        // [NUEVO] Política anti-inanición mejorada:
        // Alternar entre sexos si hay ambos esperando
        if (hombres_esperando > 0 && mujeres_esperando > 0) {
            static int turno = HOMBRE;  // [NUEVO] Variable estática para turnos
            if (turno == HOMBRE) {
                pthread_cond_broadcast(&cond_hombres);
                turno = MUJER;
            } else {
                pthread_cond_broadcast(&cond_mujeres);
                turno = HOMBRE;
            }
        } else if (hombres_esperando > 0) {
            pthread_cond_broadcast(&cond_hombres);
        } else if (mujeres_esperando > 0) {
            pthread_cond_broadcast(&cond_mujeres);
        }
    }
    
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

int main() {
    pthread_t personas[MAX_PERSONAS];
    int personas_data[MAX_PERSONAS][2]; // [id, sexo]
    
    // [NUEVO] Inicialización de semilla para números aleatorios
    srand(time(NULL));
    
    // Crear hilos para personas (alternando hombres y mujeres)
    for (int i = 0; i < MAX_PERSONAS; i++) {
        personas_data[i][0] = i + 1;
        personas_data[i][1] = (i % 2 == 0) ? HOMBRE : MUJER;  // Alternancia
        pthread_create(&personas[i], NULL, persona, &personas_data[i]);
        sleep(rand() % 2);  // [NUEVO] Llegadas aleatorias
    }
    
    // Esperar a que todas las personas terminen
    for (int i = 0; i < MAX_PERSONAS; i++) {
        pthread_join(personas[i], NULL);
    }
    
    // Limpiar recursos
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_hombres);  // [NUEVO]
    pthread_cond_destroy(&cond_mujeres);  // [NUEVO]
    
    printf("Simulación completada. Todos han usado el baño.\n");
    
    return 0;
}