PRÁCTICA 3 - SISTEMA CLIENTE-SERVIDOR CON DATASET DE SPOTIFY

INTEGRANTES: 
* Derian Santiago Rojas
* Joel Santiago Neuta
* Adriana Lucia Gonzalez

FECHA: [Fecha actual]

=================================
1. DESCRIPCIÓN GENERAL
=================================

Sistema cliente-servidor que permite consultar la duración media (en ms) de 
canciones del dataset "spotify_dataset.csv". El servidor atiende hasta 32 
clientes concurrentes mediante sockets TCP y procesos fork(). NO carga el 
dataset en memoria (lee línea por línea desde disco), por lo que el uso de 
RAM es inferior a 1 MB, cumpliendo la restricción del profesor.

=================================
2. ARCHIVOS ENTREGADOS
=================================

p3-server.c   - Código del servidor
p3-client.c   - Código del cliente  
Makefile      - Para compilar ambos programas
LEEME         - Este archivo

=================================
3. COMPILACIÓN
=================================

En la terminal, dentro de la carpeta del proyecto:

    make

Se generan los ejecutables:
    - p3-server
    - p3-client

Para limpiar:

    make clean

=================================
4. EJECUCIÓN
=================================

PASO 1 - Iniciar el servidor (una terminal):
    ./p3-server
    
    Muestra: "Servidor listo (max 32 clientes)"

PASO 2 - Iniciar cliente(s) (otra terminal):
    ./p3-client

PASO 3 - Usar el menú del cliente:

    === MENU ===
    1. Buscar artista
    2. Buscar genero
    3. Salir
    Opcion: 

    - Opción 1: Ingresar nombre de artista (ej: "BTS", "Metallica")
      Devuelve la duración media de TODAS las canciones de ese artista.

    - Opción 2: Ingresar género musical (ej: "pop", "rock")
      Devuelve la duración media de todas las canciones de ese género.

    - Opción 3: Sale del cliente.

    Después de cada búsqueda, presionar Enter para volver al menú.

=================================
5. FORMATO DE COMUNICACIÓN (interno)
=================================

Cliente envía al servidor:
    "1|nombre_artista"   → Buscar por artista
    "2|nombre_genero"    → Buscar por género
    "3|"                 → Cerrar conexión

Servidor responde con texto legible:
    "Duracion media de BTS: 234567.89 ms"
    o mensaje de error si no encuentra nada.

=================================
6. ARCHIVO DE LOG
=================================

El servidor genera "server.log" en el mismo directorio. Cada búsqueda registra:

    [YYYYMMDDTHHMMSS] Cliente IP [busqueda - tipo - termino]

Ejemplo:
    [20250514T103022] Cliente 127.0.0.1 [busqueda - artista - BTS]
    [20250514T103045] Cliente 127.0.0.1 [busqueda - genero - pop]

=================================
7. REQUISITOS DEL SISTEMA
=================================

- Linux (o WSL2 en Windows, o macOS)
- Compilador GCC
- Archivo "spotify_dataset.csv" en el mismo directorio que el servidor

Formato esperado del CSV:
    columna 0: artista
    columna 1: canción  
    columna 2: año
    columna 3: duración_ms
    columna 5: géneros (separados por comas)

Si tu CSV tiene diferente orden, ajustar índices en extraer_columna().

=================================
8. POSIBLES ERRORES
=================================

Error: "Address already in use"
    Solución: El puerto 8080 está ocupado. Esperar o matar proceso anterior:
    killall p3-server

Error: "No se pudo abrir el dataset"
    Solución: spotify_dataset.csv no existe o no tiene permisos de lectura.

Error: "Connection refused"
    Solución: El servidor no está corriendo. Ejecutar primero ./p3-server.

=================================
9. EJEMPLO DE SESIÓN
=================================

Terminal 1 (servidor):
    $ ./p3-server
    Servidor listo (max 32 clientes)

Terminal 2 (cliente):
    $ ./p3-client

    === MENU ===
    1. Buscar artista
    2. Buscar genero
    3. Salir
    Opcion: 1
    Ingrese nombre del artista: BTS

    >>> Duracion media de BTS: 234567.89 ms

    Presione Enter para continuar...

    === MENU ===
    1. Buscar artista
    2. Buscar genero
    3. Salir
    Opcion: 3
    Saliendo...

Terminal 1 (ver log):
    $ cat server.log
    [20250514T103022] Cliente 127.0.0.1 [busqueda - artista - BTS]

=================================
FIN DEL LEEME
=================================
