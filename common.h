#ifndef _COMMON_H_
#define _COMMON_H_

#define BUFFER_SIZE 768
#define PATH_MAX_SIZE 4096
#define PORT 9734
#define TRUE 1

struct file_copy_info {
    char path[PATH_MAX_SIZE];
    long size;
};

int get_file_size_in_bytes();
void show_progress();

#endif
