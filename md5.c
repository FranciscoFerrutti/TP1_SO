#include "pipe_manager.h"

#define CANT_HIJOS 1

int main(int argc, const char * argv[]){
    int current_file=1;
    char aux[60]={0};
    pid_t child_pid[CANT_HIJOS];
    int parent_to_child_pipe[CANT_HIJOS][2];
    int child_to_parent_pipe[CANT_HIJOS][2];
    fd_set readfds;
    
    for (int i = 0; i < CANT_HIJOS; i++) {
        if (pipe(parent_to_child_pipe[i]) == -1 || pipe(child_to_parent_pipe[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    
    for (int i = 0; i < CANT_HIJOS; i++) {
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

            close(4);
            dup(1);

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
    
    FD_ZERO(&readfds);
    for (int i = 0; i < CANT_HIJOS; i++) {
        FD_SET(child_to_parent_pipe[i][0], &readfds); // Add slave pipes to the set
    }

    while(current_file<argc){
        int ready = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            exit(1);
        } else if(ready>0){
            for(int i=0; i<CANT_HIJOS; i++){
                if(FD_ISSET(child_to_parent_pipe[i][0], &readfds)){
                    pipe_read(child_to_parent_pipe[i][0], aux);
                    printf("pipe content: %s\n", aux);
                    write(parent_to_child_pipe[i][1], argv[current_file], strlen(argv[current_file])+1);
                    current_file++;
                }
            }
        }
    }

    for (int i = 0; i < CANT_HIJOS; i++) {
        close(parent_to_child_pipe[i][1]);
        close(child_to_parent_pipe[i][0]);
        waitpid(child_pid[i], NULL, 0);
    }

    exit(EXIT_SUCCESS);
}
