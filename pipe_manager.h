//pipe_manager.h
#ifndef _PIPEMNG
#define _PIPEMNG

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/types.h>

#define TO_READ 1
#define TO_WRITE 2

//linux filesystem Ext4 path limitation is 4096 bytes
#define MAX_PATH 4096
#define MAX_MD5 32

int pipe_read(int fd, char *buff);
int pipe_write(int fd, const char *buff);

#endif