#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#define MAX_PATH 50
#define MAX_MD5 32

int main(){
    char buff[6]={0};
    
    read(0,buff,6);
    printf("el string: %s", buff);
    
    while(1){
        
        



        char path[]="./files/11";
        char md5[MAX_MD5+1];
        char * md5_cmd="md5sum ./%s";
        char command[MAX_PATH + strlen(md5_cmd)];

        sprintf(command, md5_cmd, path);

        FILE *fp = popen(command, "r");

        if (fp == NULL) {
            printf("slave: ERROR. Could not create fd\n");
            pclose(fp);
            exit(EXIT_FAILURE);
        }

        fgets(md5, MAX_MD5, fp);
        md5[MAX_MD5] = 0;

        printf("the md5: %s\n", md5);
        exit(0);
    }

    
}