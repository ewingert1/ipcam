#include <task.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

char recv_buffer[50];

void task1(void *arg)
{
	int bytes, fd;
	uint16_t port=5656;
	struct sockaddr_in me={0},from={0};
	socklen_t addrlen = sizeof(struct sockaddr_in);
	
	Channel* chan = (Channel*)arg;
	unsigned long val = 0;
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	me.sin_family = AF_INET;
	me.sin_addr.s_addr = INADDR_ANY;
	me.sin_port = htons(5656);
	bind(fd, (struct sockaddr*)&me, sizeof(me));
	
	while(1)
	{
		memset(recv_buffer, 0, sizeof(recv_buffer));
		fdwait(fd, 'r');
		bytes = recvfrom(fd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr*)&from, &addrlen);
		printf("bytes read = %d; val=%s, err=%s\n", bytes, recv_buffer, strerror(errno));
		int status = sendto(fd, recv_buffer, bytes, 0, (struct sockaddr*)&from, addrlen);
		//printf("sendto back = %d, err=%s\n",status, strerror(errno));
		chansendul(chan, recv_buffer[0]);
	}
}

void taskmain(int argc, char **argv)
{	
	char val=0;
	Channel* chan = chancreate(sizeof(val), 1);
	taskcreate(task1, (void*)chan, 32768);
	while(1)
	{
		val = chanrecvul(chan);
		printf("taskmain : val=%c\n", val);
	}
}
