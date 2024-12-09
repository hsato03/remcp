#ifndef _COMMON_H_
#define _COMMON_H_

#define BUFFER_SIZE 4096
#define PATH_MAX_SIZE 4096
#define CHUNK_SIZE 2048
#define PORT 9734
#define TRUE 1
#define MAX_CLIENTS 1

#define CLIENT_RECEIVE "client_receive"
#define CLIENT_SEND "client_send"

#define MAX_TRIES 5
#define FAIL 1
#define SUCCESS 2

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
void show_progress(long write, long total, char* action);
FILE* open_or_create_file(char *file_path, int sockfd);
FILE* open_file(char *file_path, int sockfd);
void send_file(int sockfd, FILE* file, long file_size, long remote_file_size, int* retries);
void terminate(int sockfd, FILE* file);
long write_to_file(int remote_sockfd, FILE* file, long bytes_written, long client_file_size);
void rename_file(char *file_path);
void add_to_number_of_clients();
void rmv_to_number_of_clients();
int get_number_of_clients();
int get_buffer_size();

#endif
