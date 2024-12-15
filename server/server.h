#ifndef _SERVER_H_
#define _SERVER_H_

void create_socket();
void initiliaze_socket();
void *handle_client(void *client_sockfd_ptr);
int send_file(int sockfd, FILE *file, long file_size, long remote_file_size, char *file_name);
long write_to_file(int remote_sockfd, FILE *file, long bytes_written, long client_file_size, char* file_name);
void increase_number_of_clients();
void decrease_number_of_clients();
int get_number_of_clients();
int get_buffer_size();

#endif
