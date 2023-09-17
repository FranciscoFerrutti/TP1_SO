all: slave.elf md5.elf vision.elf

slave.elf: slave.c pipe_manager.c
	gcc -Wall slave.c pipe_manager.c -o slave.elf -std=c99 -lm -lrt -pthread -g -D_XOPEN_SOURCE=500

md5.elf: md5.c pipe_manager.c
	gcc -Wall md5.c pipe_manager.c -o md5.elf -std=c99 -lm -lrt -pthread -g -D_XOPEN_SOURCE=500

vision.elf: vision.c pipe_manager.c
	gcc -Wall vision.c pipe_manager.c -o vision.elf -std=c99 -lm -lrt -pthread -g -D_XOPEN_SOURCE=500

clean:
	rm -f slave.elf md5.elf vision.elf resultado.txt

.PHONY: all clean
