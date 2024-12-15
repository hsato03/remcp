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

int send_file(int sockfd, FILE *file, long file_size, long remote_file_size, char *file_name) {
    int last_server_buffer_size;
    int current_server_buffer_size;
    long total_bytes_read = remote_file_size;
    size_t bytes_read;

    read(sockfd, &current_server_buffer_size, sizeof(current_server_buffer_size));

    last_server_buffer_size = current_server_buffer_size;
    char *buffer = (char *)malloc(current_server_buffer_size * sizeof(char));
    while ((bytes_read = fread(buffer, 1, current_server_buffer_size, file)) > 0) {
        if (write(sockfd, buffer, bytes_read) == -1) {
            perror("\nErro ao enviar os dados do arquivo");
            return FALSE;
        }

        total_bytes_read += bytes_read;
        show_progress(total_bytes_read, file_size, "enviados", file_name, TRUE, current_server_buffer_size);
        sleep(1);

        read(sockfd, &current_server_buffer_size, sizeof(current_server_buffer_size));
        if (current_server_buffer_size != last_server_buffer_size) {
            buffer = (char *)realloc(buffer, current_server_buffer_size * sizeof(char));
            last_server_buffer_size = current_server_buffer_size;
        }
    }
    
    free(buffer);
    return TRUE;
}

long write_to_file(int remote_sockfd, FILE *file, long bytes_written, long client_file_size, char* file_name) {
    ssize_t bytes_received;
    size_t buffer_offset = 0;
    char write_buffer[CHUNK_SIZE];
    long total_bytes_written = bytes_written;
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));

    while ((bytes_received = read(remote_sockfd, buffer, BUFFER_SIZE)) > 0) {
        for (size_t i = 0; i < bytes_received; i++) {
            write_buffer[buffer_offset++] = buffer[i];

            // Atualiza o arquivo a cada 128 bytes
            if (buffer_offset == CHUNK_SIZE) {
                if (fwrite(write_buffer, 1, CHUNK_SIZE, file) != CHUNK_SIZE) {
                    perror("\nErro ao escrever no arquivo");
                    break;
                }
                buffer_offset = 0;
                total_bytes_written += CHUNK_SIZE;
            }
        }

        show_progress(total_bytes_written, client_file_size, "escritos", file_name, TRUE, bytes_received);
        sleep(1);
    }

    if (buffer_offset > 0) {
        if (fwrite(write_buffer, 1, buffer_offset, file) != buffer_offset) {
            perror("\nErro ao escrever o restante no arquivo");
        }
        total_bytes_written += buffer_offset;
        show_progress(total_bytes_written, client_file_size, "escritos", file_name, TRUE, bytes_received);
    }

    free(buffer);
    return total_bytes_written;
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
    remove_trailing_slashes(request_info.file_path);
    strncpy(request_info.type, CLIENT_RECEIVE, sizeof(request_info.type) - 1);
    request_info.bytes_written = get_file_size_in_bytes(file);

    int response;
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

        if (read(sockfd, &response, sizeof(response)) < 0) {
            perror("Erro ao receber dado");
            retries++;
            continue;
        }

        if (response == FAIL) {
            retries++;
            continue;
        }

        if (write(sockfd, &request_info, sizeof(request_info)) == -1) {
            perror("Erro ao enviar as informações do arquivo.");
            retries++;
            continue;
        }

        long server_file_size;
        read(sockfd, &server_file_size, sizeof(server_file_size));

        long total_bytes_write = write_to_file(sockfd, file, request_info.bytes_written, server_file_size, file_name);
        if (total_bytes_write == server_file_size) {
            rename_file(client_path);
            printf("\nArquivo salvo com sucesso\n");
            return;
        }
        // retries = 0;
    }
}

void client_to_server_transfer(char **argv) {
    split_server_info(argv[2], &server_ip, &server_path);
    client_path = argv[1];
    remove_trailing_slashes(client_path);

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

    int response;
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

        if (read(sockfd, &response, sizeof(response)) < 0) {
            perror("Erro ao receber dado");
            retries++;
            continue;
        }

        if (response == FAIL) {
            retries++;
            continue;
        }

        if (write(sockfd, &request_info, sizeof(request_info)) == -1) {
            perror("Erro ao enviar as informações do arquivo.");
            retries++;
            continue;
        }

        long server_file_size;
        read(sockfd, &server_file_size, sizeof(server_file_size));

        // Pula os bytes ja transferidos
        fseek(file, server_file_size, SEEK_SET);

        if (send_file(sockfd, file, client_file_size, server_file_size, file_name) < 1) {
            retries = 0;
            continue;
        }

        printf("\nArquivo enviado com sucesso.\n");
        return;
    }
}

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

    validate_input(argc);

    if (strstr(argv[1], ":") != NULL) {
        server_to_client_transfer(argv);
    } else {
        client_to_server_transfer(argv);
    }

    terminate(sockfd, file);
    return 0;
}
