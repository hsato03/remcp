#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "client.h"

int sockfd;
struct sockaddr_in address;

void create_socket(char *server_ip) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(server_ip);
    address.sin_port = htons(PORT);
}

void validate_input(int parameters_size) {
    if (parameters_size < 3) {
        printf("É necessário passar 2 parametros: o arquivo que será copiado e o destino.\n");
        printf("Exemplo: ./remcp meu_arquivo.txt 192.168.0.5:/home/usuario/teste\n");
        exit(1);
    }
}

int main(int argc, char **argv) {
    validate_input(argc);

    char *file_name = argv[1];
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        printf("Arquivo %s não encontrado.\n", file_name);
        exit(1);
    }

    long client_file_size = get_file_size_in_bytes(file);

    char *destination = argv[2];
    char *destination_ip;
    char *destination_path;
    if (strstr(destination, ":") != NULL) {
        destination_ip = strtok(destination, ":");
        destination_path = strtok(NULL, ":");
    } else {
        destination_path = destination;
    }

    if (strstr(file_name, "/") != NULL) {
        char *last_token;
        char *token = strtok(file_name, "/");
        
        while (token != NULL) {
            last_token = token;
            token = strtok(NULL, "/");
        }

        file_name = last_token;
    }

    strcat(destination_path, file_name);
    strcat(destination_path, ".part");

    struct file_copy_info file_info;
    strncpy(file_info.path, destination_path, sizeof(file_info.path) - 1);
    file_info.size = client_file_size;

    printf("IP de destino: %s\n", destination_ip);
    printf("Caminho de destino: %s\n", file_info.path);
    printf("TAMANHO ARQUIVO CLIENTE: %ld \n", file_info.size);

    create_socket(destination_ip);

    if (connect(sockfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("oops: client1");
        exit(1);
    }

    if (write(sockfd, &file_info, sizeof(file_info)) == -1) {
        perror("Erro ao enviar as informações do arquivo.");
        fclose(file);
        close(sockfd);
        exit(1);
    }

    long server_file_size;
    read(sockfd, &server_file_size, sizeof(server_file_size));
    printf("TAMANHO ARQUIVO SERVIDOR: %ld \n", server_file_size);

    fseek(file, server_file_size, SEEK_SET);

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
