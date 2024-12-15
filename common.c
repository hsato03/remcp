#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>
#include "common.h"

void terminate(int sockfd, FILE *file) {
    close(sockfd);
    if (file != NULL) {
        if (flock(fileno(file), LOCK_UN) == -1) {
            perror("Erro ao liberar o lock");
        }
        fclose(file);
    }
    pthread_exit(NULL);
}

int get_file_size_in_bytes(FILE *file) {
    fseek(file, 0, SEEK_END);
    return ftell(file);
}

void show_progress(long write, long total, char* action, char* file_name, int is_client, int transfer_rate) {
    int progress = (int)((double)write / total * 50);
    printf("\r[");
    for (int i = 0; i < 50; i++) {
        if (i < progress)
            printf("#");
        else
            printf(" ");
    }

    printf("] %ld/%ld bytes %s", write, total, action);
    printf(" --- %d Bps --- %s", transfer_rate, file_name);
    if (is_client) {
        fflush(stdout);
    } else {
        printf("\n");
    }    
}

FILE *open_or_create_file(char *file_path, int sockfd) {
    FILE *file = fopen(file_path, "rb+");
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

FILE *open_file(char *file_path, int sockfd) {
    FILE *file = fopen(file_path, "r");
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

void rename_file(char *file_path) {
    char new_file_name[PATH_MAX_SIZE];
    strcpy(new_file_name, file_path);
    new_file_name[strlen(new_file_name) - 5] = '\0';
    rename(file_path, new_file_name);
}

void remove_trailing_slashes(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '/') {
        str[len - 1] = '\0';
        remove_trailing_slashes(str);
    }
}

char *get_file_name_from_path(char *file_path) {
    if (strstr(file_path, "/") != NULL) {
        char *last_token;
        char *token = strtok(strdup(file_path), "/");

        while (token != NULL) {
            last_token = token;
            token = strtok(NULL, "/");
        }

        return last_token;
    }

    remove_trailing_slashes(file_path);
    return file_path;
}
