#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 768
#define PORT 9734

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("É necessário passar 2 parametros: o arquivo que será copiado e o destino.\n");
        printf("Exemplo: ./remcp meu_arquivo.txt 192.168.0.5:/home/usuario/teste\n");
        exit(1);
    }

    char *file_name = argv[1];
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        printf("Arquivo %s não encontrado.\n", file_name);
        exit(1);
    }

    char *destination = argv[2];
    char *destination_ip;
    char *destination_path;
    if (strstr(destination, ":") != NULL) {
        destination_ip = strtok(destination, ":");
        destination_path = strtok(NULL, ":");
    } else {
        destination_path = destination;
    }

    strcat(destination_path, file_name);
    strcat(destination_path, ".part");

    printf("IP de destino: %s\n", destination_ip);
    printf("Caminho de destino: %s\n", destination_path);

    int sockfd;
    struct sockaddr_in address;
    int result;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(destination_ip);
    address.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("oops: client1");
        exit(1);
    }

    if (write(sockfd, destination_path, strlen(destination_path) + 1) == -1) {
        perror("Erro ao enviar o nome do arquivo.");
        fclose(file);
        close(sockfd);
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (write(sockfd, buffer, bytes_read) == -1) {
            perror("Erro ao enviar os dados do arquivo");
            close(sockfd);
            fclose(file);
            exit(1);
        }
        sleep(1);
    }

    printf("Arquivo enviado com sucesso.\n");
    close(sockfd);
    fclose(file);
    exit(0);
}
