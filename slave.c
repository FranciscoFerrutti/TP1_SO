//slave.c
#include "pipe_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    char path[MAX_PATH] = {0};
    char md5[MAX_MD5 + 1];
    char md5_cmd[MAX_PATH + 9]; // Adjust the size accordingly
    strcpy(md5_cmd, "md5sum %s"); // Adjust the command as needed

    while (1) {
        int ready = pipe_read(STDIN_FILENO, path);
        if (ready == -1) {
            perror("pipe_read");
            exit(EXIT_FAILURE);
        }

        if (path[0] == 0) {
            // Exit when there are no more files to read
            exit(EXIT_SUCCESS);
        }

        sprintf(md5_cmd, "md5sum %s", path);

        FILE *fp = popen(md5_cmd, "r");
        if (fp == NULL) {
            perror("popen");
            exit(EXIT_FAILURE);
        }

        fgets(md5, MAX_MD5, fp);
        md5[MAX_MD5] = 0;
        pclose(fp);

        pipe_write(STDOUT_FILENO, md5);
    }

    // No need to exit here, as it will exit when there are no more files to read.

    return EXIT_SUCCESS;
}
