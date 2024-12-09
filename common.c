#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/file.h>
#include "common.h"

void terminate(int sockfd, FILE* file) {
    close(sockfd);
    if (file != NULL) {
        if (flock(fileno(file), LOCK_UN) == -1) {
            perror("Erro ao liberar o lock");
        }
        fclose(file);
    }
    pthread_exit(NULL);
}

int get_file_size_in_bytes(FILE* file) {
    fseek(file, 0, SEEK_END);
    return ftell(file);
}

void show_progress(long write, long total, char* action) {
    int progress = (int)((double)write / total * 50);
    printf("\r[");
    for (int i = 0; i < 50; i++) {
        if (i < progress)
            printf("#");
        else
            printf(" ");
    }

    printf("] %ld/%ld bytes %s", write, total, action);
    // printf("Quantidade de clientes: %d ", get_number_of_clients());
    printf(" --- Bytes/s/cliente: %d --- THREAD: %i\n", get_buffer_size(), gettid());
    // fflush(stdout);
    // system("clear");
}

FILE* open_or_create_file(char *file_path, int sockfd) {
    FILE* file = fopen(file_path, "rb+");
    if (file) {
        printf("Arquivo existente encontrado.\n");
    } else {
        printf("Arquivo não encontrado. Criando novo.\n");
        file = fopen(file_path, "wb");
        if (file == NULL) {
            perror("Erro ao criar o arquivo");
            terminate(sockfd, file);
        }
    }

    if (flock(fileno(file), LOCK_EX) == -1) {
        perror("Erro ao aplicar o lock");
        terminate(sockfd, file);
    }

    return file;
}

FILE* open_file(char *file_path, int sockfd) {
    FILE* file = fopen(file_path, "r");
    if (!file) {
        printf("Arquivo %s não encontrado.\n", file_path);
        if (sockfd != -1) {
            close(sockfd);
        }
        pthread_exit(NULL);
    }

    if (flock(fileno(file), LOCK_EX) == -1) {
        perror("Erro ao aplicar o lock");
        terminate(sockfd, file);
    }


    return file;
}

int number_of_clients = 0;
pthread_mutex_t mutex;

void add_to_number_of_clients() {
    pthread_mutex_lock(&mutex);
    number_of_clients++;
    pthread_mutex_unlock(&mutex);
}

void rmv_to_number_of_clients() {
    pthread_mutex_lock(&mutex);
    number_of_clients--;
    pthread_mutex_unlock(&mutex);
}

int get_number_of_clients() {
    pthread_mutex_lock(&mutex);
    int temp = number_of_clients;
    pthread_mutex_unlock(&mutex);
    return temp;
}

int get_buffer_size(){
    return (int)(BUFFER_SIZE/get_number_of_clients());
}

void send_file(int sockfd, FILE* file, long file_size, long remote_file_size, int* retries) {
    long total_bytes_read = remote_file_size;
    char* buffer = (char *)malloc(get_buffer_size() * sizeof(char));
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, get_buffer_size(), file)) > 0) {
        printf("bytes lidos por vez: %d\n", bytes_read);
        printf("buffer size: %d\n", get_buffer_size());

        if (write(sockfd, buffer, bytes_read) == -1) {
            perror("\nErro ao enviar os dados do arquivo");
            if(retries != NULL){
                *retries = 1;
                return;
            } else {
                rmv_to_number_of_clients();
                terminate(sockfd, file);
            }
        }

        total_bytes_read += bytes_read;
        show_progress(total_bytes_read, file_size, "enviados");
        sleep(1);
        buffer = (char *)realloc(buffer, get_buffer_size() * sizeof(char));
    }
    free(buffer);
}

long write_to_file(int remote_sockfd, FILE* file, long bytes_written, long client_file_size) {
    ssize_t bytes_received;
    size_t buffer_offset = 0;
    char write_buffer[CHUNK_SIZE];
    long total_bytes_written = bytes_written;
    char *file_chunks = (char *)malloc(get_buffer_size() * sizeof(char));

    while ((bytes_received = read(remote_sockfd, file_chunks, get_buffer_size())) > 0) {
        for (size_t i = 0; i < bytes_received; i++) {
            write_buffer[buffer_offset++] = file_chunks[i];

            // Atualiza o arquivo a cada 128 bytes
            if (buffer_offset == CHUNK_SIZE) {
                if (fwrite(write_buffer, 1, CHUNK_SIZE, file) != CHUNK_SIZE) {
                    perror("\nErro ao escrever no arquivo");
                    break;
                }
                buffer_offset = 0;
                total_bytes_written += CHUNK_SIZE;
                show_progress(total_bytes_written, client_file_size, "escritos");
            }
        }

        sleep(1);
        file_chunks = (char *)realloc(file_chunks, get_buffer_size() * sizeof(char));
        printf("Bytes recebidos: %zu", bytes_received);
    }

    if (buffer_offset > 0) {
        if (fwrite(write_buffer, 1, buffer_offset, file) != buffer_offset) {
            perror("\nErro ao escrever o restante no arquivo");
        }
        total_bytes_written += buffer_offset;
        show_progress(total_bytes_written, client_file_size, "escritos");
    }
    free(file_chunks);
    return total_bytes_written;
}

void rename_file(char *file_path) {
    printf("\nRENOMEANDO\n");
    char new_file_name[get_buffer_size()];
    strcpy(new_file_name, file_path);
    new_file_name[strlen(new_file_name)-5] = '\0';
    rename(file_path, new_file_name);
}
