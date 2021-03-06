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


#include "server_msg.h" 
#include "client_msg.h" 

#define STACK 32768
#define PORT 5656

#define BUFFER_SIZE	65536
#define FRAME_CHUNK	1400

typedef struct _server_ctx
{
	int fd_serv;
	int fd;
	Channel *chan;
} server_ctx;

server_ctx* server_ctx_create(Channel *chan);
void server_ctx_destroy(server_ctx *ctx);
void send_mesg(server_ctx *ctx, uint8_t opcode, uint8_t *data, uint64_t length);
void send_frame(server_ctx *ctx, uint8_t *ptr, uint64_t length);
void servertask(void *arg);
