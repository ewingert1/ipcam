 
#include <task.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

void taskmain(int argc, char **argv)
{	
	uint8_t buffer[65536];
	int fd, bytes;
	
	fd = netdial(UDP, "127.0.0.1", 5656);
	printf("fd=%d\n", fd);
	
	bytes = read(STDIN_FILENO, buffer, sizeof(buffer));
	if(bytes > 0)
	{
		write(fd, buffer, bytes);
	}
	taskexit(0);
}
