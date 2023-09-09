#include "pipe_manager.h"

int main(){
    /*char buff[6]={0};
    
    read(0,buff,6);
    printf("el string: %s\n", buff);
    exit(0);
    */
//
    write(4, "hola\n", 5);
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    char path[MAX_PATH]={0};
    char md5[MAX_MD5+1];
    char * md5_cmd="md5sum ./%s";
    char command[MAX_PATH + strlen(md5_cmd)];
    char aux[2]={0};
    aux[0]='2';

    while(1){
        //should we use select here for waiting for the next path?
        write(STDOUT_FILENO, aux, 2);
        int ready = select(1,&read_fds,NULL, NULL, NULL);
        if(ready==-1){
            perror("select");
            exit(EXIT_FAILURE);
        }
        
        //now, i want to read what father sent me

        pipe_read(STDIN_FILENO, path);
        
        if(path[0]==0){
            exit(EXIT_SUCCESS);
        }
        
        sprintf(command, md5_cmd, path);
            
        FILE *fp = popen(command, "r");
        if (fp == NULL) { // we should consider not exiting when a wrong path is sent
            pclose(fp);
            exit(EXIT_FAILURE);
        }

        fgets(md5, MAX_MD5, fp);
        md5[MAX_MD5] = 0;

        pipe_write(STDOUT_FILENO, md5);
        exit(EXIT_SUCCESS);
    }

    
}