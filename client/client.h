#ifndef _CLIENT_H_
#define _CLIENT_H_

void create_socket(char *server_ip);
void validate_input(int parameters_size);
void split_server_info(char* server_info, char** server_ip, char** server_path);
void server_to_client_transfer(char **argv);
void client_to_server_transfer(char **argv);
int send_file(int sockfd, FILE *file, long file_size, long remote_file_size, char *file_name);
long write_to_file(int remote_sockfd, FILE *file, long bytes_written, long client_file_size, char* file_name);

#endif
