#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>
#include "server.h"
#include "../common.h"

int server_sockfd, client_sockfd;
struct sockaddr_in server_address;
struct sockaddr_in client_address;
const int succes_code = SUCCESS;
const int fail_code = FAIL;
int number_of_clients = 0;
pthread_mutex_t mutex;

void create_socket() {
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(PORT);
}

void initiliaze_socket() {
    bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
    listen(server_sockfd, MAX_CLIENTS);
}

void increase_number_of_clients() {
    pthread_mutex_lock(&mutex);
    number_of_clients++;
    pthread_mutex_unlock(&mutex);
}

void decrease_number_of_clients() {
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

int get_buffer_size() {
    return (int)(BUFFER_SIZE / get_number_of_clients());
}

int send_file(int sockfd, FILE *file, long file_size, long remote_file_size, char *file_name) {
    int last_num_of_clients = get_number_of_clients();
    int current_num_of_clients = -1;
    long total_bytes_read = remote_file_size;
    char *buffer = (char *)malloc(get_buffer_size() * sizeof(char));
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, get_buffer_size(), file)) > 0) {
        if (write(sockfd, buffer, bytes_read) == -1) {
            perror("\nErro ao enviar os dados do arquivo");
            return FALSE;
        }

        total_bytes_read += bytes_read;
        show_progress(total_bytes_read, file_size, "enviados", file_name, FALSE, get_buffer_size());
        sleep(1);

        current_num_of_clients = get_number_of_clients();
        if (current_num_of_clients != last_num_of_clients) {
            buffer = (char *)realloc(buffer, get_buffer_size() * sizeof(char));
            last_num_of_clients = current_num_of_clients;
        }
    }

    free(buffer);
    return TRUE;
}

long write_to_file(int remote_sockfd, FILE *file, long bytes_written, long client_file_size, char* file_name) {
    int last_num_of_clients = get_number_of_clients();
    int current_num_of_clients = -1;
    ssize_t bytes_received;
    size_t buffer_offset = 0;
    char write_buffer[CHUNK_SIZE];
    long total_bytes_written = bytes_written;
    int buffer_size = get_buffer_size();
    char *buffer = (char *)malloc(buffer_size * sizeof(char));

    write(remote_sockfd, &buffer_size, sizeof(buffer_size));

    while ((bytes_received = read(remote_sockfd, buffer, get_buffer_size())) > 0) {
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

        buffer_size = get_buffer_size();    
        write(remote_sockfd, &buffer_size, sizeof(buffer_size));
        
        show_progress(total_bytes_written, client_file_size, "escritos", file_name, FALSE, buffer_size);
        sleep(1);

        current_num_of_clients = get_number_of_clients();
        if (current_num_of_clients != last_num_of_clients) {
            buffer = (char *)realloc(buffer, get_buffer_size() * sizeof(char));
            last_num_of_clients = current_num_of_clients;
        }
    }

    if (buffer_offset > 0) {
        if (fwrite(write_buffer, 1, buffer_offset, file) != buffer_offset) {
            perror("\nErro ao escrever o restante no arquivo");
        }
        total_bytes_written += buffer_offset;
        show_progress(total_bytes_written, client_file_size, "escritos", file_name, FALSE, get_buffer_size());
    }

    free(buffer);
    return total_bytes_written;
}

void *handle_client(void *client_sockfd_ptr) {
    increase_number_of_clients();
    int client_sockfd = *(int *)client_sockfd_ptr;
    free(client_sockfd_ptr);
    struct copy_request request_info;
    long file_size;
    FILE *file;
    int fd;

    // Recebe as informações da transferência
    if (read(client_sockfd, &request_info, sizeof(request_info)) <= 0) {
        perror("Cliente desconectado ou erro ao ler dados");
        close(client_sockfd);
        pthread_exit(NULL);
    }

    char* file_name = get_file_name_from_path(request_info.file_path);

    if (strcmp(request_info.type, CLIENT_SEND) == 0) {
        strcat(request_info.file_path, ".part");
        file = open_or_create_file(request_info.file_path, client_sockfd);

        file_size = get_file_size_in_bytes(file);

        // Envia a quantidade de bytes já escrita para o cliente
        write(client_sockfd, &file_size, sizeof(file_size));

        long total_bytes_write = write_to_file(client_sockfd, file, file_size, request_info.file_size, file_name);
        if (total_bytes_write == request_info.file_size) {
            rename_file(request_info.file_path);
        }
    } else {
        file = open_file(request_info.file_path, client_sockfd);
        file_size = get_file_size_in_bytes(file);
        fseek(file, request_info.bytes_written, SEEK_SET);

        // Envia o tamanho total do arquivo para o cliente
        write(client_sockfd, &file_size, sizeof(file_size));

        send_file(client_sockfd, file, file_size, request_info.bytes_written, file_name);
    }

    decrease_number_of_clients();

    terminate(client_sockfd, file);
}

int main() {
    // Ignora SIGPIPE para evitar que o servidor termine ao escrever em um socket fechado
    signal(SIGPIPE, SIG_IGN);

    create_socket();
    initiliaze_socket();

    int client_len = sizeof(client_address);
    pthread_t client_thread;
    while (TRUE) {
        printf("server waiting\n");
        int *client_sockfd_ptr = malloc(sizeof(int));
        *client_sockfd_ptr = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
        if (*client_sockfd_ptr < 0) {
            perror("Erro ao aceitar conexão");
            free(client_sockfd_ptr);
            continue;
        }
        
        if (get_number_of_clients() >= MAX_CLIENTS) {
            if (write(*client_sockfd_ptr, &fail_code, sizeof(fail_code)) < 0) {
                perror("Erro ao enviar dado de erro");
            }

            close(*client_sockfd_ptr);
            free(client_sockfd_ptr);
            continue;
        }

        if (write(*client_sockfd_ptr, &succes_code, sizeof(succes_code)) < 0) {
            perror("Erro ao enviar dado de sucesso");
        }

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
