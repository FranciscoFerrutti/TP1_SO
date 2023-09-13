//slave.c
#include "pipe_manager.h"

int main() {
    char path[MAX_PATH] = {0};
    char md5[MAX_MD5 + 1];
    char *md5_cmd = "md5sum ./%s";
    char command[MAX_PATH + strlen(md5_cmd)];

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

        sprintf(command, md5_cmd, path);

        FILE *fp = popen(command, "r");
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
