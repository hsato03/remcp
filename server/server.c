#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
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

int main() {
    struct copy_request request_info;
    char file_chunks[BUFFER_SIZE];
    long file_size;
    FILE* file;
    int client_len = sizeof(client_address);

    create_socket();
    initiliaze_socket();

    while (TRUE) {
        printf("server waiting\n");
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
        
        // Recebe as informações da transferência
        read(client_sockfd, &request_info, sizeof(request_info));
        printf("request file name: %s\n", request_info.file_path);
        printf("request file size: %ld\n", request_info.file_size);
        printf("TIPO: %s\n", request_info.type);
        printf("BYTES WRITTEN: %ld\n", request_info.bytes_written);

        if (strcmp(request_info.type, CLIENT_SEND) == 0) {
            strcat(request_info.file_path, ".part");
            file = open_or_create_file(request_info.file_path);
            if (file == NULL) {
                perror("Erro ao criar o arquivo.");
                exit(1);
            }

            file_size = get_file_size_in_bytes(file);

            // Envia a quantidade de bytes já escrita para o cliente
            write(client_sockfd, &file_size, sizeof(file_size));

            long total_bytes_write = write_to_file(client_sockfd, file, file_size, file_chunks, request_info.file_size);
            if (total_bytes_write == request_info.file_size) {
                rename_file(request_info.file_path);
            }
        } else {
            file = open_file(request_info.file_path);
            file_size = get_file_size_in_bytes(file);
            fseek(file, 0, SEEK_SET);

            // Envia o tamanho total do arquivo para o cliente
            write(client_sockfd, &file_size, sizeof(file_size));

            printf("TAMANHO ARQUIVO SERVIDOR %ld\n", file_size);
            send_file(client_sockfd, file, file_size, request_info.bytes_written);
        }

        fclose(file);
        close(client_sockfd);
    }
}
