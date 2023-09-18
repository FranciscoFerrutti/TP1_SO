all: slave.elf md5.elf vision.elf

slave.elf: slave.c pipe_manager.c
	gcc -Wall slave.c pipe_manager.c -o slave.elf -std=c99 -lm -lrt -pthread -g -D_XOPEN_SOURCE=500

md5.elf: md5.c pipe_manager.c
	gcc -Wall md5.c pipe_manager.c -o md5.elf -std=c99 -lm -lrt -pthread -g -D_XOPEN_SOURCE=500

vision.elf: vision.c pipe_manager.c
	gcc -Wall vision.c pipe_manager.c -o vision.elf -std=c99 -lm -lrt -pthread -g -D_XOPEN_SOURCE=500

clean:
	rm -f slave.elf md5.elf vision.elf resultado.txt PVS-Studio.log report.tasks strace_out
	rm -r .config

#Si tienen problemas al hacer make pvs-studio, hagan un make clean y vuelvan a intentar
pvs-studio:
	pvs-studio-analyzer credentials PVS-Studio Free FREE-FREE-FREE-FREE
	pvs-studio-analyzer trace -- make -j8
	pvs-studio-analyzer analyze -j2 -l /root/.config/PVS-Studio/PVS-Studio.lic -o PVS-Studio.log
	plog-converter -a GA:1,2 -t tasklist -o report.tasks PVS-Studio.log

.PHONY: all clean
