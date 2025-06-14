# Documentación de Cambios - Implementación del Baño Unisex

## Características Principales

### 1. Configuración del Sistema
- Se definió una capacidad reducida del baño a 2 personas (`CAPACIDAD 2`)
- Se implementaron constantes para identificar géneros:
  - `HOMBRE = 1`
  - `MUJER = 0`

### 2. Variables de Control
- `banio_sexo`: Control del género actual usando el baño (-1 indica vacío)
- `hombres_dentro` y `mujeres_dentro`: Contadores de ocupación
- Sistema FIFO implementado con:
  - `siguiente_turno`: Para asignar turnos
  - `turno_actual`: Para controlar el orden de entrada

### 3. Sincronización
- Uso de mutex para exclusión mutua
- Implementación de variable de condición para control de entrada
```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_entrada = PTHREAD_COND_INITIALIZER;
```

### 4. Simulación de Uso
- Función `usar_banio()` que simula el uso con tiempo aleatorio
- Mensajes de entrada y salida para seguimiento

### 5. Control de Acceso
Implementación de reglas de acceso:
1. Respeto del orden FIFO
2. Control de capacidad máxima
3. Exclusividad de género cuando el baño está en uso

### 6. Gestión de Personas
- Array de 10 personas con orden predefinido
- Distribución de género: 7 hombres y 3 mujeres
- Orden de llegada establecido: `{H1, M2, H3, H4, H5, M6, H7, M8, H9, H10}`

## Mejoras Implementadas

1. **Sistema FIFO**
   - Garantiza que las personas entren en el orden de llegada
   - Evita inanición de cualquier género

2. **Control de Capacidad**
   - Límite de 2 personas simultáneas
   - Verificación antes de permitir entrada

3. **Notificación Eficiente**
   - Broadcast de señales cuando:
     - El baño se vacía
     - Hay espacio disponible para el mismo género

4. **Gestión de Memoria**
   - Liberación apropiada de recursos
   - Destrucción de mutex y variables de condición

## Funcionamiento
1. Cada persona toma un turno al llegar
2. Espera si:
   - No es su turno
   - El baño está lleno
   - El baño está siendo usado por el género opuesto
3. Al entrar:
   - Actualiza contadores
   - Establece el género actual del baño
4. Al salir:
   - Actualiza contadores
   - Notifica a los demás si pueden entrar 