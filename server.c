#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
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

void open_file(FILE **file, char *file_path) {
    *file = fopen(file_path, "rb+");
    if (*file) {
        printf("Arquivo existente encontrado.\n");
        return;
    } 

    printf("Arquivo não encontrado. Criando novo.\n");
    *file = fopen(file_path, "wb");
    if (file == NULL) {
        perror("Erro ao criar arquivo.");
        close(client_sockfd);
    }
}

long write_to_file(FILE* file, long bytes_write, char *file_chunks, long client_file_size) {
    ssize_t bytes_received;
    size_t buffer_offset = 0;
    char write_buffer[CHUNK_SIZE];
    long total_bytes_write = bytes_write;
    while ((bytes_received = read(client_sockfd, file_chunks, BUFFER_SIZE)) > 0) {
        for (size_t i = 0; i < bytes_received; i++) {
            write_buffer[buffer_offset++] = file_chunks[i];

            // Atualiza o arquivo a cada 128 bytes
            if (buffer_offset == CHUNK_SIZE) {
                if (fwrite(write_buffer, 1, CHUNK_SIZE, file) != CHUNK_SIZE) {
                    perror("Erro ao escrever no arquivo");
                    break;
                }
                buffer_offset = 0;
                total_bytes_write += CHUNK_SIZE;
                show_progress(total_bytes_write, client_file_size);
            }
        }
    }

    if (buffer_offset > 0) {
        if (fwrite(write_buffer, 1, buffer_offset, file) != buffer_offset) {
            perror("Erro ao escrever o restante no arquivo");
        }
        total_bytes_write += buffer_offset;
        show_progress(total_bytes_write, client_file_size);
    }

    return total_bytes_write;
}

void rename_file(char *file_path) {
    printf("\nRENOMEANDO\n");
    char new_file_name[BUFFER_SIZE];
    strcpy(new_file_name, file_path);
    new_file_name[strlen(new_file_name)-5] = '\0';
    rename(file_path, new_file_name);
}

int main() {
    struct file_copy_info file_info;
    char file_chunks[BUFFER_SIZE];
    long file_size;
    FILE* file;
    int client_len = sizeof(client_address);

    create_socket();
    initiliaze_socket();

    while (TRUE) {
        printf("server waiting\n");
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
        
        // Recebe as informações do arquivo do cliente
        read(client_sockfd, &file_info, sizeof(file_info));
        printf("file name: %s\n", file_info.path);
        printf("file size: %ld\n", file_info.size);

        open_file(&file, file_info.path);
        file_size = get_file_size_in_bytes(file);

        // Envia a quantidade de bytes já escrito para o cliente
        write(client_sockfd, &file_size, sizeof(file_size));

        long total_bytes_write = write_to_file(file, file_size, file_chunks, file_info.size);
        if (total_bytes_write == file_info.size) {
            rename_file(file_info.path);
        }

        fclose(file);
        close(client_sockfd);
    }
}
