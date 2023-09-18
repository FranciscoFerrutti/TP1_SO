//pipe_manager.h
#ifndef _PIPEMNG
#define _PIPEMNG

#include <sys/wait.h>
#include <sys/select.h>

int pipe_read(int fd, char *buff);
int pipe_write(int fd, const char *buff);

#endif