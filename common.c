#include <stdio.h>
#include "common.h"

int get_file_size_in_bytes(FILE* file) {
    fseek(file, 0, SEEK_END);
    return ftell(file);
}

void show_progress(long write, long total) {
    int progress = (int)((double)write / total * 50);
    printf("\r[");
    for (int i = 0; i < 50; i++) {
        if (i < progress)
            printf("#");
        else
            printf(" ");
    }

    printf("] %ld/%ld bytes escritos", write, total);
    fflush(stdout);
}
