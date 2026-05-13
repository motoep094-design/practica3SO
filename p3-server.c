/*
 * p3-server.c
 * Servidor que calcula duración media por artista o por género.
 * Recibe comandos: "1|artista" o "2|genero"
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

void escribir_log(const char *ip, int tipo, const char *termino) {
    FILE *f = fopen(LOGFILE, "a");
    if (!f) return;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    const char *tipo_str = (tipo == 1) ? "artista" : "genero";
    fprintf(f, "[%04d%02d%02dT%02d%02d%02d] Cliente %s [busqueda - %s - %s]\n",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            ip, tipo_str, termino);
    fclose(f);
}

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
    while (d > 0 && (dest[d-1] == ' ' || dest[d-1] == '\t'))
        dest[--d] = '\0';
}

// Duplicar string para poder modificar con strtok
char *duplicar_string(const char *src) {
    char *dst = malloc(strlen(src) + 1);
    if (dst) strcpy(dst, src);
    return dst;
}

// Verifica si un género está en la lista de géneros (columna 5)
int contiene_genero(const char *generos_csv, const char *genero_buscar) {
    char *copia = duplicar_string(generos_csv);
    if (!copia) return 0;
    char *token = strtok(copia, ",");
    int encontrado = 0;
    while (token != NULL) {
        // recortar espacios
        while (*token == ' ' || *token == '\t') token++;
        char *fin = token + strlen(token) - 1;
        while (fin > token && (*fin == ' ' || *fin == '\t')) fin--;
        *(fin + 1) = '\0';
        
        if (strcmp(token, genero_buscar) == 0) {
            encontrado = 1;
            break;
        }
        token = strtok(NULL, ",");
    }
    free(copia);
    return encontrado;
}

// Calcula duración media por ARTISTA
double duracion_media_artista(const char *artista) {
    FILE *f = fopen(DATASET, "r");
    if (!f) return -1;
    char linea[2048];
    long suma = 0;
    int count = 0;
    
    fgets(linea, sizeof(linea), f); // saltar cabecera si existe
    while (fgets(linea, sizeof(linea), f)) {
        char art[256], dur_str[20];
        extraer_columna(linea, 0, art, sizeof(art));
        extraer_columna(linea, 3, dur_str, sizeof(dur_str));
        if (strcmp(art, artista) == 0) {
            suma += atoi(dur_str);
            count++;
        }
    }
    fclose(f);
    if (count == 0) return -1;
    return (double)suma / count;
}

// Calcula duración media por GÉNERO
double duracion_media_genero(const char *genero) {
    FILE *f = fopen(DATASET, "r");
    if (!f) return -1;
    char linea[2048];
    long suma = 0;
    int count = 0;
    
    fgets(linea, sizeof(linea), f); // saltar cabecera
    while (fgets(linea, sizeof(linea), f)) {
        char generos_csv[1024], dur_str[20];
        extraer_columna(linea, 5, generos_csv, sizeof(generos_csv));
        extraer_columna(linea, 3, dur_str, sizeof(dur_str));
        if (contiene_genero(generos_csv, genero)) {
            suma += atoi(dur_str);
            count++;
        }
    }
    fclose(f);
    if (count == 0) return -1;
    return (double)suma / count;
}

void atender_cliente(int cliente_sock, struct sockaddr_in addr) {
    char buffer[BUFFER_SIZE];
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(cliente_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) break;
        
        int opcion;
        char termino[512];
        sscanf(buffer, "%d|%[^\n]", &opcion, termino);
        
        double media;
        char respuesta[BUFFER_SIZE];
        
        switch (opcion) {
            case 1: // Buscar artista
                media = duracion_media_artista(termino);
                if (media < 0)
                    snprintf(respuesta, BUFFER_SIZE, "No se encontraron canciones del artista '%s'", termino);
                else
                    snprintf(respuesta, BUFFER_SIZE, "Duracion media de %s: %.2f ms", termino, media);
                send(cliente_sock, respuesta, strlen(respuesta), 0);
                escribir_log(ip, 1, termino);
                break;
            case 2: // Buscar género
                media = duracion_media_genero(termino);
                if (media < 0)
                    snprintf(respuesta, BUFFER_SIZE, "No se encontraron canciones del genero '%s'", termino);
                else
                    snprintf(respuesta, BUFFER_SIZE, "Duracion media del genero %s: %.2f ms", termino, media);
                send(cliente_sock, respuesta, strlen(respuesta), 0);
                escribir_log(ip, 2, termino);
                break;
            case 3: // Salir
                close(cliente_sock);
                return;
            default:
                send(cliente_sock, "Opcion invalida", 15, 0);
        }
    }
    close(cliente_sock);
}

int main() {
    int server_fd, cliente_sock;
    struct sockaddr_in server_addr, cliente_addr;
    socklen_t addrlen = sizeof(cliente_addr);
    int clientes_activos = 0;
    
    signal(SIGCHLD, SIG_IGN);
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PUERTO);
    
    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, MAX_CLIENTES);
    
    printf("Servidor listo (max %d clientes)\n", MAX_CLIENTES);
    
    while (clientes_activos < MAX_CLIENTES) {
        cliente_sock = accept(server_fd, (struct sockaddr*)&cliente_addr, &addrlen);
        if (fork() == 0) {
            close(server_fd);
            atender_cliente(cliente_sock, cliente_addr);
            exit(0);
        } else {
            close(cliente_sock);
            clientes_activos++;
        }
    }
    close(server_fd);
    return 0;
}
