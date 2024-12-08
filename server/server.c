#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/file.h>
#include "../common.h"
#include "server.h"

int server_sockfd, client_sockfd;
struct sockaddr_in server_address;
struct sockaddr_in client_address;

void create_socket() {
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(PORT);
}

void initiliaze_socket() {
    bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
    listen(server_sockfd, 5);
}

void* handle_client(void* client_sockfd_ptr) {
    int client_sockfd = *(int*)client_sockfd_ptr;
    free(client_sockfd_ptr);
    struct copy_request request_info;
    char file_chunks[BUFFER_SIZE];
    long file_size;
    FILE* file;
    int fd;

    // Recebe as informações da transferência
    if (read(client_sockfd, &request_info, sizeof(request_info)) <= 0) {
        perror("Cliente desconectado ou erro ao ler dados");
        close(client_sockfd);
        pthread_exit(NULL);
    }
    printf("request file name: %s\n", request_info.file_path);
    printf("request file size: %ld\n", request_info.file_size);
    printf("TIPO: %s\n", request_info.type);
    printf("BYTES WRITTEN: %ld\n", request_info.bytes_written);

    if (strcmp(request_info.type, CLIENT_SEND) == 0) {
        strcat(request_info.file_path, ".part");
        file = open_or_create_file(request_info.file_path, client_sockfd);

        fd = fileno(file);
        if (flock(fd, LOCK_EX) == -1) {
            perror("Erro ao aplicar o lock");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        file_size = get_file_size_in_bytes(file);

        // Envia a quantidade de bytes já escrita para o cliente
        write(client_sockfd, &file_size, sizeof(file_size));

        long total_bytes_write = write_to_file(client_sockfd, file, file_size, file_chunks, request_info.file_size);
        if (total_bytes_write == request_info.file_size) {
            rename_file(request_info.file_path);
        }
    } else {
        file = open_file(request_info.file_path, client_sockfd);
        file_size = get_file_size_in_bytes(file);
        fseek(file, request_info.bytes_written, SEEK_SET);

        // Envia o tamanho total do arquivo para o cliente
        write(client_sockfd, &file_size, sizeof(file_size));

        printf("TAMANHO ARQUIVO SERVIDOR %ld\n", file_size);
        send_file(client_sockfd, file, file_size, request_info.bytes_written);
    }

    fclose(file);
    close(client_sockfd);
    pthread_exit(NULL);
}

int main() {
    // Ignora SIGPIPE para evitar que o servidor termine ao escrever em um socket fechado
    signal(SIGPIPE, SIG_IGN);

    create_socket();
    initiliaze_socket();

    int client_len = sizeof(client_address);
    pthread_t client_thread;
    while (TRUE) {
        printf("Executando no loop principal (main).\n");
        printf("server waiting\n");
        int* client_sockfd_ptr = malloc(sizeof(int));
        *client_sockfd_ptr = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
        if (*client_sockfd_ptr < 0) {
            perror("Erro ao aceitar conexão");
            free(client_sockfd_ptr);
            continue;
        }

        // TODO: Adicionar padrão throttling e retry
        if (pthread_create(&client_thread, NULL, handle_client, client_sockfd_ptr) != 0) {
            perror("Erro ao criar thread.");
            close(*client_sockfd_ptr);
            free(client_sockfd_ptr);
            continue;
        }

        // Limpa os recursos da thread automaticamente ao finalizar
        pthread_detach(client_thread);
    }

    close(server_sockfd);
    return 0;
}
