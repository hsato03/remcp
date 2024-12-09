#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include "../common.h"
#include "client.h"

int sockfd;
struct sockaddr_in address;

void create_socket(char *server_ip)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(server_ip);
    address.sin_port = htons(PORT);
}

void validate_input(int parameters_size)
{
    if (parameters_size < 3)
    {
        printf("É necessário passar 2 parametros: o arquivo que será copiado e o destino.\n");
        printf("Exemplo: ./remcp meu_arquivo.txt 192.168.0.5:/home/usuario/teste\n");
    }
}

char *get_file_name_from_path(char *file_path)
{
    if (strstr(file_path, "/") != NULL)
    {
        char *last_token;
        char *token = strtok(strdup(file_path), "/");

        while (token != NULL)
        {
            last_token = token;
            token = strtok(NULL, "/");
        }

        return last_token;
    }

    return file_path;
}

void split_server_info(char *server_info, char **server_ip, char **server_path)
{
    *server_ip = strtok(strdup(server_info), ":");
    *server_path = strchr(server_info, ':') + 1;
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);

    add_to_number_of_clients();
    validate_input(argc);

    char *client_path;
    char *server_ip;
    char *server_path;
    FILE *file;

    if (strstr(argv[1], ":") != NULL)
    {
        split_server_info(argv[1], &server_ip, &server_path);
        client_path = argv[2];

        char *file_name = get_file_name_from_path(server_path);
        strcat(client_path, file_name);
        strcat(client_path, ".part");

        file = open_or_create_file(client_path, sockfd);
        if (file == NULL)
        {
            perror("Erro ao criar o arquivo.");
        }

        struct copy_request request_info;
        strncpy(request_info.file_path, server_path, sizeof(request_info.file_path) - 1);
        strncpy(request_info.type, CLIENT_RECEIVE, sizeof(request_info.type) - 1);
        request_info.bytes_written = get_file_size_in_bytes(file);

        printf("IP servidor: %s\n", server_ip);
        printf("Caminho servidor: %s\n", request_info.file_path);
        printf("TAMANHO ARQUIVO CLIENTE: %ld \n", request_info.file_size);
        printf("TOTAL JA ESCRITO: %ld \n", request_info.bytes_written);

        create_socket(server_ip);

        if (connect(sockfd, (struct sockaddr *)&address, sizeof(address)) == -1)
        {
            perror("oops: client1");
            printf("oops: client1");
            terminate(sockfd, file);
        }

        if (write(sockfd, &request_info, sizeof(request_info)) == -1)
        {
            perror("Erro ao enviar as informações do arquivo.");
            terminate(sockfd, file);
        }

        long server_file_size;
        read(sockfd, &server_file_size, sizeof(server_file_size));

        long total_bytes_write = write_to_file(sockfd, file, request_info.bytes_written, server_file_size);
        if (total_bytes_write == server_file_size)
        {
            rename_file(client_path);
        }
    }
    else
    {
        split_server_info(argv[2], &server_ip, &server_path);
        client_path = argv[1];

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
        int temp = 0;
        while (retries < MAX_TRIES)
        {
            temp = 0;
            if (retries > 0)
            {
                printf("Erro ao se comunicar com o servidor. Retentando em %i segundos\n", retries * 4);
                sleep(retries * 5);
            }
            
            create_socket(server_ip);

            if (connect(sockfd, (struct sockaddr *)&address, sizeof(address)) == -1)
            {
                perror("oops: client1");
                retries++;
                continue;
            }

            int response;
            if (read(sockfd, &response, sizeof(response)) < 0)
            {
                perror("Erro ao receber dado");
                retries++;
                continue;
            }
            else
            {
                response = ntohl(response);
                printf("Número recebido: %d\n", response);
            }

            if (response == FAIL)
            {
                retries++;
                continue;
            }

            if (write(sockfd, &request_info, sizeof(request_info)) == -1)
            {
                perror("Erro ao enviar as informações do arquivo.");
                terminate(sockfd, file);
            }

            long server_file_size;
            read(sockfd, &server_file_size, sizeof(server_file_size));
            printf("TAMANHO ARQUIVO SERVIDOR: %ld \n", server_file_size);

            // Pula os bytes ja transferidos
            fseek(file, server_file_size, SEEK_SET);

            send_file(sockfd, file, client_file_size, server_file_size, &temp);

            if (temp != 0)
                continue;
            
            printf("\nArquivo enviado com sucesso.\n");
            terminate(sockfd, file);
        }
    }

    terminate(sockfd, file);
    return 0;
}
