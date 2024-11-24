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
#define PATH_MAX_SIZE 4096

struct file_copy_info {
    char path[PATH_MAX_SIZE];
    long size;
};

int main() {
    int server_sockfd, client_sockfd;
    int server_len, client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    struct file_copy_info file_info;
    char file_chunks[BUFFER_SIZE];
    FILE* file;

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
        
        read(client_sockfd, &file_info, sizeof(file_info));
        printf("file name: %s\n", file_info.path);
        printf("file size: %ld\n", file_info.size);

        long file_size;
        file = fopen(file_info.path, "r+");
        if (file) {
            printf("OPENED\n");
            fseek(file, 0, SEEK_END);
            file_size = ftell(file);
        } else {
            printf("CREATED\n");
            file = fopen(file_info.path, "w+");
            if (file == NULL) {
                perror("Erro ao criar o arquivo.");
                close(client_sockfd);
                continue;
            }       
        }

        write(client_sockfd, &file_size, sizeof(file_size));

        ssize_t bytes_received;
        size_t buffer_offset = 0;
        char write_buffer[CHUNK_SIZE];
        long total_bytes_write = file_size;
        while ((bytes_received = read(client_sockfd, file_chunks, BUFFER_SIZE)) > 0) {
            for (size_t i = 0; i < bytes_received; i++) {
                write_buffer[buffer_offset++] = file_chunks[i];

                if (buffer_offset == CHUNK_SIZE) {
                    if (fwrite(write_buffer, 1, CHUNK_SIZE, file) != CHUNK_SIZE) {
                        perror("Erro ao escrever no arquivo");
                        break;
                    }
                    buffer_offset = 0;
                    total_bytes_write += CHUNK_SIZE;
                }
            }
        }

        if (buffer_offset > 0) {
            if (fwrite(write_buffer, 1, buffer_offset, file) != buffer_offset) {
                perror("Erro ao escrever o restante no arquivo");
            }
            total_bytes_write += buffer_offset;
        }

        if (total_bytes_write == file_info.size) {
            printf("RENOMEANDO");
            char new_file_name[BUFFER_SIZE];
            strcpy(new_file_name, file_info.path);
            new_file_name[strlen(new_file_name)-5] = '\0';
            rename(file_info.path, new_file_name);
        }

        fclose(file);
        close(client_sockfd);
    }
}
