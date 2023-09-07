#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define CANT_HIJOS 5

int main(){
    int i;
    pid_t child_pid[CANT_HIJOS];
    int parent_to_child_pipe[CANT_HIJOS][2];
    int child_to_parent_pipe[CANT_HIJOS][2];
    
    for (i = 0; i < CANT_HIJOS; i++) {
        if (pipe(parent_to_child_pipe[i]) == -1 || pipe(child_to_parent_pipe[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    
    for (i = 0; i < CANT_HIJOS; i++) {
        child_pid[i] = fork();

        if (child_pid[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (child_pid[i] == 0) {
            
            close(parent_to_child_pipe[i][1]);
            close(child_to_parent_pipe[i][0]);

            //code goes here
            char *args[] = {"./slave.elf", NULL}; 
            execve("./slave.elf", args, NULL);
            perror("Error en execve");


            close(parent_to_child_pipe[i][0]);
            close(child_to_parent_pipe[i][1]);
            exit(EXIT_SUCCESS);
        } else {
            
            close(parent_to_child_pipe[i][0]);
            close(child_to_parent_pipe[i][1]);
        }
    }

    for (i = 0; i < CANT_HIJOS; i++) {
        close(parent_to_child_pipe[i][1]);
        close(child_to_parent_pipe[i][0]);
        waitpid(child_pid[i], NULL, 0);
    }

    return 0;
}
