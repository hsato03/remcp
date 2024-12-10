#ifndef _CLIENT_H_
#define _CLIENT_H_

void create_socket(char *server_ip);
void validate_input(int parameters_size);
char* get_file_name_from_path(char* file_path);
void split_server_info(char* server_info, char** server_ip, char** server_path);
void server_to_client_transfer(char **argv);
void client_to_server_transfer(char **argv);

#endif
