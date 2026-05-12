/*
 * p3-client.c
 * Cliente que se conecta al servidor, muestra un menú y envía comandos.
 * Formato de comandos: "1|nombre_artista", "2|", "3|"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PUERTO 8080
#define BUFFER_SIZE 1024

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PUERTO);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }
    
    int opcion;
    char entrada[512];
    char buffer[BUFFER_SIZE];
    
    do {
        printf("\n====================================\n");
        printf("    CALCULADOR DE DURACION MEDIA\n");
        printf("====================================\n");
        printf("1. Ingresar artista\n");
        printf("2. Buscar duracion media de todas sus canciones\n");
        printf("3. Salir\n");
        printf("Opcion: ");
        scanf("%d", &opcion);
        getchar();  // consumir newline
        
        switch (opcion) {
            case 1:
                printf("Nombre del artista: ");
                fgets(entrada, sizeof(entrada), stdin);
                entrada[strcspn(entrada, "\n")] = '\0';
                snprintf(buffer, BUFFER_SIZE, "1|%s", entrada);
                send(sock, buffer, strlen(buffer), 0);
                recv(sock, buffer, BUFFER_SIZE, 0);
                printf("-> Artista guardado: %s\n", entrada);
                break;
                
            case 2:
                send(sock, "2|", 3, 0);
                memset(buffer, 0, BUFFER_SIZE);
                recv(sock, buffer, BUFFER_SIZE, 0);
                printf("\n>>> %s\n", buffer);
                break;
                
            case 3:
                send(sock, "3|", 3, 0);
                printf("Saliendo...\n");
                break;
                
            default:
                printf("Opcion no valida. Intente de nuevo.\n");
        }
        
        if (opcion != 3) {
            printf("\nPresione Enter para continuar...");
            getchar();
        }
    } while (opcion != 3);
    
    close(sock);
    return 0;
}