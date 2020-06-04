#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <task.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#include "client_msg.h"
#include "server_msg.h"

#define HEIGHT 		480
#define WIDTH 		640
#define FRAMESIZE	HEIGHT*WIDTH*2
#define BUFFSIZE	65536

uint8_t frame[FRAMESIZE];	
uint8_t buffer[BUFFSIZE];	

int running = 1;

void irq_handler(int dummy)
{
	running = 0;
}

void taskmain(int argc, char **argv)
{	
	uint8_t buffer[65536];
	int fd, bytes;
	struct sockaddr_in from = {0};
	socklen_t fromlen = sizeof(from);
	
	struct timeval tv1, tv2;
	
	uint8_t hellomsg[] = {HELLO, 0, 0}, startmsg[] = {START_STREAM, 0, 0}, stopmsg[] = {STOP_STREAM, 0, 0};
	
	signal(SIGINT, irq_handler);
	
	fd = netdial(UDP, "127.0.0.1", 5656);
	printf("fd=%d\n", fd);
	
	write(fd, hellomsg, sizeof(hellomsg));
	write(fd, startmsg, sizeof(startmsg));

	
	gettimeofday(&tv1, NULL);
	puts("eyop");
	while(running)
	{
		int bytes = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, &fromlen);
		if(bytes>0 && buffer[0]==FRAME_DONE)
		{
			gettimeofday(&tv2, NULL);
    		printf("fps = %f\n", 1000000./((tv2.tv_sec*1000000 + tv2.tv_usec) - (tv1.tv_sec*1000000 + tv1.tv_usec)));
    		gettimeofday(&tv1, NULL);    	
		}
	}
	
	write(fd, stopmsg, sizeof(stopmsg));		
	sleep(1);
	taskexit(0);
}
