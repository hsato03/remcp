#ifndef _COMMON_H_
#define _COMMON_H_

#define BUFFER_SIZE 2048
#define PATH_MAX_SIZE 4096
#define CHUNK_SIZE 128
#define PORT 9734
#define TRUE 1
#define CLIENT_RECEIVE "client_receive"
#define CLIENT_SEND "client_send"

struct copy_request {
    char file_path[PATH_MAX_SIZE];
    long file_size;
    char type[15];
    long bytes_written;
};

struct server_response {
    long server_sockfd;
    long file_size;
};

int get_file_size_in_bytes(FILE* file);
void show_progress(long write, long total, char* action);
FILE* open_or_create_file(char *file_path);
FILE* open_file(char *file_path);
void send_file(int sockfd, FILE* file, long file_size, long remote_file_size);
void terminate(int sockfd, FILE* file, int status_code);
long write_to_file(int remote_sockfd, FILE* file, long bytes_written, char *file_chunks, long client_file_size);
void rename_file(char *file_path);

#endif
