#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/select.h>

#define CANT_HIJOS 5

int main(int argc, const char * argv[]){
    int current_file=1;
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

            char *args[] = {"./slave.elf", NULL}; 
            execve("./slave.elf", args, NULL);
            perror("Error in execve");


            //close(parent_to_child_pipe[i][0]);
            //close(child_to_parent_pipe[i][1]);
            exit(EXIT_SUCCESS);
        }      
        
    }
    
    char buff[6]={0};
    buff[0]='h';
    buff[1]='o';
    buff[2]='l';
    buff[3]='a';
    buff[4]='?';
    for(int i=0; i<CANT_HIJOS; i++){
        write(parent_to_child_pipe[i][1], buff, 6);
    }
    exit(EXIT_SUCCESS);
    FD_ZERO(&readfds);
    for (int i = 0; i < CANT_HIJOS; i++) {
        FD_SET(child_to_parent_pipe[i][0], &readfds); // Add slave pipes to the set
    }

/*
    while(1){
        int ready = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            exit(1);
        } else if(ready>0){
            for(int i=0; i<CANT_HIJOS; i++){
                if(FD_ISSET(child_to_parent_pipe[i][0], &readfds)){
                    write(parent_to_child_pipe[i][1], argv[current_file], strlen(argv[current_file])+1);
                    current_file++;
                }
            }
        }
        
    }
*/
    for (int i = 0; i < CANT_HIJOS; i++) {
        close(parent_to_child_pipe[i][1]);
        close(child_to_parent_pipe[i][0]);
        waitpid(child_pid[i], NULL, 0);
    }

    exit(EXIT_SUCCESS);
}
