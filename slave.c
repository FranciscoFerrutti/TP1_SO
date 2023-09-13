#include "pipe_manager.h"

int main(){

    char path[MAX_PATH]={0};
    char md5[MAX_MD5+1];
    char * md5_cmd="md5sum ./%s";
    char command[MAX_PATH + strlen(md5_cmd)];


    while(1){

        int ready = pipe_read(STDIN_FILENO, path);
        if(ready==-1){
            perror("select");
            exit(EXIT_FAILURE);
        }

        /* 
        if(path[0]==0){
            exit(EXIT_SUCCESS); WHEN THERE IS NO MORE FILES TO READ, THE MASTER CLOSES ITS WRITE PIPE, AND THE SLAVE SHOULD EXIT ON ITS OWN WHEN IT READS EOF
        } THEREFORE, THIS IS NOT NEEDED AND SHOULD BE DELETED
        */
        sprintf(command, md5_cmd, path);
            
        FILE *fp = popen(command, "r");
        if (fp == NULL) { // we should consider not exiting when a wrong path is sent
            pclose(fp);
            exit(EXIT_FAILURE);
        }

        fgets(md5, MAX_MD5, fp);
        md5[MAX_MD5] = 0;
        pclose(fp);

        pipe_write(STDOUT_FILENO, md5);
        exit(EXIT_SUCCESS);
    }

    
}