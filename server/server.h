#ifndef _SERVER_H_
#define _SERVER_H_

void create_socket();
void initiliaze_socket();
void *handle_client(void *client_sockfd_ptr);

#endif
