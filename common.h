#ifndef _COMMON_H_
#define _COMMON_H_

#define BUFFER_SIZE 4096
#define PATH_MAX_SIZE 4096
#define CHUNK_SIZE 128
#define PORT 9734
#define TRUE 1
#define FALSE 0
#define MAX_CLIENTS 3

#define CLIENT_RECEIVE "client_receive"
#define CLIENT_SEND "client_send"

#define MAX_RETRIES 2
#define SUCCESS 1
#define FAIL 0

struct copy_request {
    char file_path[PATH_MAX_SIZE];
    long file_size;
    char type[15];
    long bytes_written;
};

struct server_response {
    long server_sockfd;
    int status_code;
};

int get_file_size_in_bytes(FILE* file);
void show_progress(long write, long total, char* action, char* file_name, int is_client, int transfer_rate);
FILE* open_or_create_file(char *file_path, int sockfd);
FILE* open_file(char *file_path, int sockfd);
void terminate(int sockfd, FILE* file);
void rename_file(char *file_path);
char* get_file_name_from_path(char* file_path);
void remove_trailing_slashes(char *str);

#endif
