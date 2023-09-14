#include "pipe_manager.h"

#define CHILD_QTY 1
#define STARTING_FILE 1
#define INITIAL_FILES_PER_CHILD 2

int first_files_per_child(int argc, const char * argv[], int parent_to_child_pipe[CHILD_QTY][2], int child_to_parent_pipe[CHILD_QTY][2]);

int main(int argc, const char * argv[]){
    
    char child_md5[CHILD_QTY][MAX_MD5+1];
    pid_t child_pid[CHILD_QTY];
    int parent_to_child_pipe[CHILD_QTY][2];
    int child_to_parent_pipe[CHILD_QTY][2];
    
    for (int i = 0; i < CHILD_QTY; i++) {
        if (pipe(parent_to_child_pipe[i]) == -1 || pipe(child_to_parent_pipe[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    
    for (int i = 0; i < CHILD_QTY; i++) {
        child_pid[i] = fork();

        if (child_pid[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (child_pid[i] == 0) {

            close(parent_to_child_pipe[i][1]);
            close(child_to_parent_pipe[i][0]);

            //code goes here
            close(STDIN_FILENO);
            dup(parent_to_child_pipe[i][0]);
            close(parent_to_child_pipe[i][0]);

            close(STDOUT_FILENO);
            dup(child_to_parent_pipe[i][1]);
            close(child_to_parent_pipe[i][1]);


            char *args[] = {"./slave.elf", NULL}; 
            execve("./slave.elf", args, NULL);
            perror("execve");


            close(parent_to_child_pipe[i][0]);
            close(child_to_parent_pipe[i][1]);
            exit(EXIT_SUCCESS);
        }
        
    }

    for (int i = 0; i < CHILD_QTY; i++) {
        close(parent_to_child_pipe[i][0]);
        close(child_to_parent_pipe[i][1]);
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    int max_fd=-1;

    for (int i = 0; i < CHILD_QTY; i++) {
        if(child_to_parent_pipe[i][0]>max_fd){
            max_fd = child_to_parent_pipe[i][0];
        }
        FD_SET(child_to_parent_pipe[i][0], &readfds); // Add slave pipes to the set
    }

    if(max_fd<0){
        perror("file descriptor");
        exit(EXIT_FAILURE);
    }


    int current_file=1;
    first_files_per_child(argc, argv, parent_to_child_pipe, child_to_parent_pipe);
    while(current_file<argc){
        int ready = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            exit(1);
        } else if(ready>0){
            for(int i=0; i<CHILD_QTY && current_file<argc; i++){
                if(FD_ISSET(child_to_parent_pipe[i][0], &readfds)){
                    pipe_read(child_to_parent_pipe[i][0], child_md5[i]);
                    printf("pipe content from child %d: %s\n", i, child_md5[i]);

                    if(current_file<argc){
                        write(parent_to_child_pipe[i][1], argv[current_file], strlen(argv[current_file])+1);
                        current_file++;
                    }

                }
            }
        } else {
            printf("ready=%d", ready);
            sleep(100000);
        }
    }

    for (int i = 0; i < CHILD_QTY; i++) {
        close(parent_to_child_pipe[i][1]);
        close(child_to_parent_pipe[i][0]);
        waitpid(child_pid[i], NULL, 0);
    } // remember: when every writing-end of a pipe is closed, the process with the reading-end finishes on its own
    //printf("******\n");
    exit(EXIT_SUCCESS);
}

int first_files_per_child(int argc, const char * argv[], int parent_to_child_pipe[CHILD_QTY][2], int child_to_parent_pipe[CHILD_QTY][2]){

    int i;
    for(i=STARTING_FILE; i<argc && i<=INITIAL_FILES_PER_CHILD*CHILD_QTY; i++){
        pipe_write(parent_to_child_pipe[i%CHILD_QTY][1], argv[i]);
    }


    return i;
}
