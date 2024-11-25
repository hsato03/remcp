#ifndef _SERVER_H_
#define _SERVER_H_

#define CHUNK_SIZE 128

void create_socket();
void initiliaze_socket();
long write_to_file(FILE* file, long bytes_write, char *file_chunks, long client_file_size);
void rename_file(char *file_path);
void open_file(FILE **file, char *file_path);

#endif
