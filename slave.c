//slave.c
#include "pipe_manager.h"

int main() {
    char path[MAX_PATH] = {0};
    char md5[MAX_MD5 + MAX_PATH+ 1];
    char *md5_cmd = "md5sum ./%s";
    char command[MAX_PATH + strlen(md5_cmd)];
    int ready = 1;

    while (ready > 0) {
        ready = pipe_read(STDIN_FILENO, path);
        if (ready == -1) {
            perror("pipe_read");
            exit(EXIT_FAILURE);
        }

        if (path[0] == 0) {
            // No more files to read, exit the loop
            break;
        }

        sprintf(command, md5_cmd, path);

        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            perror("popen");
            exit(EXIT_FAILURE);
        }

        fgets(md5, MAX_MD5+strlen(path)+5, fp);
        md5[MAX_MD5+strlen(path)+6] = '\0';
        pclose(fp);

        // Write the MD5 hash to the parent process
        pipe_write(STDOUT_FILENO, md5);
    }

    // Close the write end of the pipe before exiting
    close(STDOUT_FILENO);

    exit(EXIT_SUCCESS);
}
