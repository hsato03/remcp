#include <arpa/inet.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "client.h"
#include "../common.h"

int sockfd;
struct sockaddr_in address;
char *client_path;
char *server_ip;
char *server_path;
FILE *file;

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
    }
}

void split_server_info(char *server_info, char **server_ip, char **server_path) {
    *server_ip = strtok(strdup(server_info), ":");
    *server_path = strchr(server_info, ':') + 1;
}

void server_to_client_transfer(char **argv) {
    split_server_info(argv[1], &server_ip, &server_path);
    client_path = argv[2];

    char *file_name = get_file_name_from_path(server_path);
    strcat(client_path, file_name);
    strcat(client_path, ".part");

    file = open_or_create_file(client_path, sockfd);
    if (file == NULL) {
        perror("Erro ao criar o arquivo.");
    }

    struct copy_request request_info;
    strncpy(request_info.file_path, server_path, sizeof(request_info.file_path) - 1);
    remove_trailing_slash(request_info.file_path);
    strncpy(request_info.type, CLIENT_RECEIVE, sizeof(request_info.type) - 1);
    request_info.bytes_written = get_file_size_in_bytes(file);

    printf("IP servidor: %s\n", server_ip);
    printf("Caminho servidor: %s\n", request_info.file_path);
    printf("TAMANHO ARQUIVO CLIENTE: %ld \n", request_info.file_size);
    printf("TOTAL JA ESCRITO: %ld \n", request_info.bytes_written);

    int retries = 0;
    while (retries <= MAX_RETRIES) {
        if (retries > 0) {
            printf("Erro ao se comunicar com o servidor. Retentando em %i segundos\n", retries * 4);
            sleep(retries * 5);
        }

        close(sockfd);
        create_socket(server_ip);

        if (connect(sockfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
            perror("oops: client1");
            retries++;
            continue;
        }

        int response;
        if (read(sockfd, &response, sizeof(response)) < 0) {
            perror("Erro ao receber dado");
            retries = 0;
            continue;
        } 
        response = response;
        printf("Número recebido: %d\n", response);

        if (write(sockfd, &request_info, sizeof(request_info)) == -1) {
            perror("Erro ao enviar as informações do arquivo.");
            retries = 0;
            continue;
        }

        long server_file_size;
        read(sockfd, &server_file_size, sizeof(server_file_size));
        printf("TAMANHO ARQUIVO SERVIDOR: %ld\n", server_file_size);

        long total_bytes_write = write_to_file(sockfd, file, request_info.bytes_written, server_file_size, file_name, TRUE);
        if (total_bytes_write == server_file_size) {
            rename_file(client_path);
            return;
        }
        retries = 0;
    }
}

void client_to_server_transfer(char **argv) {
    split_server_info(argv[2], &server_ip, &server_path);
    client_path = argv[1];
    remove_trailing_slash(client_path);

    file = open_file(client_path, -1);
    long client_file_size = get_file_size_in_bytes(file);
    char *file_name = get_file_name_from_path(client_path);

    // Concatena caminho de destino com o arquivo
    strcat(server_path, "/");
    strcat(server_path, file_name);

    struct copy_request request_info;
    strncpy(request_info.file_path, server_path, sizeof(request_info.file_path) - 1);
    strncpy(request_info.type, CLIENT_SEND, sizeof(request_info.type) - 1);
    request_info.file_size = client_file_size;

    printf("IP de destino: %s\n", server_ip);
    printf("Caminho de destino: %s\n", request_info.file_path);
    printf("TAMANHO ARQUIVO CLIENTE: %ld \n", request_info.file_size);

    int retries = 0;
    while (retries <= MAX_RETRIES) {
        if (retries > 0) {
            printf("Erro ao se comunicar com o servidor. Retentando em %i segundos\n", retries * 4);
            sleep(retries * 5);
        }

        close(sockfd);
        create_socket(server_ip);

        if (connect(sockfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
            perror("oops: client1");
            retries++;
            continue;
        }

        int response;
        if (read(sockfd, &response, sizeof(response)) < 0) {
            perror("Erro ao receber dado");
            retries = 0;
            continue;
        } 
        printf("Número recebido: %d\n", response);
    

        if (response == FAIL) {
            retries = 0;
            continue;
        }

        if (write(sockfd, &request_info, sizeof(request_info)) == -1) {
            perror("Erro ao enviar as informações do arquivo.");
            retries = 0;
            continue;
        }

        long server_file_size;
        read(sockfd, &server_file_size, sizeof(server_file_size));
        printf("TAMANHO ARQUIVO SERVIDOR: %ld \n", server_file_size);

        // Pula os bytes ja transferidos
        fseek(file, server_file_size, SEEK_SET);

        if (send_file(sockfd, file, client_file_size, server_file_size, file_name, TRUE) < 1) {
            retries = 0;
            continue;
        }

        printf("\nArquivo enviado com sucesso.\n");
        return;
    }
}

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

    increase_number_of_clients();
    validate_input(argc);

    // TODO: testar a transferencia com entradas em diferentes formatos
    //          1. ./client.out remcp/meuarquivo.txt/ 127.0.0.1:/tmp/
    //          2. ./client.out remcp/meuarquivo.txt/ 127.0.0.1:/tmp
    //          3. ./client.out remcp/meuarquivo.txt 127.0.0.1:/tmp/
    //          4. ./client.out remcp/meuarquivo.txt 127.0.0.1:/tmp
    if (strstr(argv[1], ":") != NULL) {
        server_to_client_transfer(argv);
    } else {
        client_to_server_transfer(argv);
    }

    terminate(sockfd, file);
    return 0;
}
