/*
 * p3-server.c
 * Servidor que calcula la duración media de todas las canciones de un artista
 * a partir de un archivo CSV (spotify_dataset.csv).
 * Escucha en puerto 8080, acepta hasta 32 clientes concurrentes con fork().
 * No usa índices en memoria, lee el archivo línea por línea.
 * Registra cada búsqueda en server.log.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>

#define PUERTO 8080
#define MAX_CLIENTES 32
#define BUFFER_SIZE 1024
#define DATASET "spotify_dataset.csv"
#define LOGFILE "server.log"

// Escribe en el archivo log con el formato exigido
void escribir_log(const char *ip, const char *artista) {
    FILE *f = fopen(LOGFILE, "a");
    if (!f) return;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    fprintf(f, "[%04d%02d%02dT%02d%02d%02d] Cliente %s [busqueda - %s - ]\n",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            ip, artista);
    fclose(f);
}

// Extrae una columna de una línea CSV (versión simple, sin comillas)
// col: 0=artista, 1=cancion, 2=año, 3=duracion_ms
void extraer_columna(const char *linea, int col, char *dest, size_t max) {
    int col_actual = 0;
    size_t i = 0, d = 0;
    dest[0] = '\0';
    while (linea[i] && col_actual <= col) {
        if (linea[i] == ',') {
            col_actual++;
            i++;
            continue;
        }
        if (col_actual == col && linea[i] != '\n' && d < max - 1) {
            dest[d++] = linea[i];
        }
        i++;
    }
    dest[d] = '\0';
    // Recortar espacios al final
    while (d > 0 && (dest[d-1] == ' ' || dest[d-1] == '\t'))
        dest[--d] = '\0';
}

// Calcula la duración media de todas las canciones de un artista
// Recorre el archivo completo línea por línea, suma duraciones y cuenta.
// Retorna -1 si no encuentra ninguna coincidencia.
double duracion_media_artista(const char *artista) {
    FILE *f = fopen(DATASET, "r");
    if (!f) {
        perror("No se pudo abrir el dataset");
        return -1;
    }
    char linea[2048];
    long suma = 0;
    int count = 0;
    
    // Saltar posible cabecera (si la primera línea contiene "artista" o "Artist")
    if (fgets(linea, sizeof(linea), f)) {
        if (strstr(linea, "artista") || strstr(linea, "Artist") ||
            strstr(linea, "track") || strstr(linea, "name")) {
            // Es cabecera, la ignoramos
        } else {
            // La primera línea ya es dato, volver al inicio
            rewind(f);
        }
    }
    
    while (fgets(linea, sizeof(linea), f)) {
        char art[256];
        char dur_str[20];
        extraer_columna(linea, 0, art, sizeof(art));
        extraer_columna(linea, 3, dur_str, sizeof(dur_str));
        
        if (strcmp(art, artista) == 0) {
            int duracion = atoi(dur_str);
            if (duracion > 0) {
                suma += duracion;
                count++;
            }
        }
    }
    fclose(f);
    if (count == 0) return -1;
    return (double)suma / count;
}

// Atiende a un cliente individual (se ejecuta en un proceso hijo)
void atender_cliente(int cliente_sock, struct sockaddr_in addr) {
    char buffer[BUFFER_SIZE];
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
    
    char artista[256] = "";
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(cliente_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) break;
        
        int opcion;
        char dato[512];
        // Formato esperado: "opcion|dato" (para opción 1) o "2|" (buscar) o "3|" (salir)
        sscanf(buffer, "%d|%[^\n]", &opcion, dato);
        
        switch (opcion) {
            case 1:  // Ingresar artista
                strncpy(artista, dato, sizeof(artista) - 1);
                artista[sizeof(artista) - 1] = '\0';
                send(cliente_sock, "OK", 2, 0);
                break;
            case 2:  // Buscar duración media
                if (artista[0] == '\0') {
                    send(cliente_sock, "Primero ingrese un artista (opcion 1)", 34, 0);
                } else {
                    double media = duracion_media_artista(artista);
                    char respuesta[BUFFER_SIZE];
                    if (media < 0)
                        snprintf(respuesta, BUFFER_SIZE,
                                 "No se encontraron canciones para el artista '%s'", artista);
                    else
                        snprintf(respuesta, BUFFER_SIZE,
                                 "Duracion media de las canciones de %s: %.2f ms",
                                 artista, media);
                    send(cliente_sock, respuesta, strlen(respuesta), 0);
                    // Registrar en el log
                    escribir_log(ip, artista);
                }
                break;
            case 3:  // Salir
                close(cliente_sock);
                return;
            default:
                send(cliente_sock, "Comando invalido", 16, 0);
        }
    }
    close(cliente_sock);
}

int main() {
    int server_fd, cliente_sock;
    struct sockaddr_in server_addr, cliente_addr;
    socklen_t addrlen = sizeof(cliente_addr);
    int clientes_activos = 0;
    
    // Ignorar SIGCHLD para evitar procesos zombie
    signal(SIGCHLD, SIG_IGN);
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PUERTO);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    if (listen(server_fd, MAX_CLIENTES) < 0) {
        perror("listen");
        exit(1);
    }
    
    printf("Servidor escuchando en puerto %d (max %d clientes)...\n", PUERTO, MAX_CLIENTES);
    
    while (clientes_activos < MAX_CLIENTES) {
        cliente_sock = accept(server_fd, (struct sockaddr*)&cliente_addr, &addrlen);
        if (cliente_sock < 0) {
            perror("accept");
            continue;
        }
        
        pid_t pid = fork();
        if (pid == 0) {  // Proceso hijo
            close(server_fd);
            atender_cliente(cliente_sock, cliente_addr);
            exit(0);
        } else if (pid > 0) { // Proceso padre
            close(cliente_sock);
            clientes_activos++;
        } else {
            perror("fork");
            close(cliente_sock);
        }
    }
    
    close(server_fd);
    return 0;
}