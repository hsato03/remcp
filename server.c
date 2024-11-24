#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define PORT 9734
#define BUFFER_SIZE 768
#define CHUNK_SIZE 128

int main() {
    int server_sockfd, client_sockfd;
    int server_len, client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    char file_name[BUFFER_SIZE];
    char file_chunks[BUFFER_SIZE];
    FILE* file;
    long file_size;

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(PORT);
    server_len = sizeof(server_address);

    bind(server_sockfd, (struct sockaddr *)&server_address, server_len);
    listen(server_sockfd, 5);

    // TODO: continuar a transferencia de um arquivo ja existente
    while(1) {		
        printf("server waiting\n");
        client_len = sizeof(client_address);
        client_sockfd = accept(server_sockfd,(struct sockaddr *)&client_address, &client_len);
        read(client_sockfd, &file_name, BUFFER_SIZE);
        printf("file name: %s\n", file_name);

        file = fopen(file_name, "r+");
        if (file) {
            printf("OPENED");
            fseek(file, 0, SEEK_END);
            file_size = ftell(file);
        } else {
            printf("CREATED");
            file = fopen(file_name, "w");
            if (file == NULL) {
                perror("Erro ao criar o arquivo.");
                close(client_sockfd);
                continue;
            }       
        }

        ssize_t bytes_received;
        size_t buffer_offset = 0;
        char write_buffer[CHUNK_SIZE];
        while ((bytes_received = read(client_sockfd, file_chunks, BUFFER_SIZE)) > 0) {
            for (size_t i = 0; i < bytes_received; i++) {
                write_buffer[buffer_offset++] = file_chunks[i];

                if (buffer_offset == CHUNK_SIZE) {
                    if (fwrite(write_buffer, 1, CHUNK_SIZE, file) != CHUNK_SIZE) {
                        perror("Erro ao escrever no arquivo");
                        break;
                    }
                    buffer_offset = 0;
                }
            }
        }

        if (buffer_offset > 0) {
            if (fwrite(write_buffer, 1, buffer_offset, file) != buffer_offset) {
                perror("Erro ao escrever o restante no arquivo");
            }
        }

        char new_file_name[BUFFER_SIZE];
        strcpy(new_file_name, file_name);
        new_file_name[strlen(new_file_name)-5] = '\0';
        rename(file_name, new_file_name);

        fclose(file);
        close(client_sockfd);
    }
}
