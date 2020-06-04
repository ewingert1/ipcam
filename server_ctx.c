#include "server_ctx.h"



typedef void (*mesg_handler)(server_ctx *ctx, uint8_t*,uint16_t);

uint8_t send_buffer[BUFFER_SIZE];
uint8_t recv_buffer[BUFFER_SIZE];
uint8_t frame_buffer[FRAME_CHUNK];

void handle_hello_msg				(server_ctx *ctx, uint8_t *data, uint16_t length);
void handle_bye_msg					(server_ctx *ctx, uint8_t *data, uint16_t length);
void handle_setframerate_msg		(server_ctx *ctx, uint8_t *data, uint16_t length);
void handle_setresolution_msg		(server_ctx *ctx, uint8_t *data, uint16_t length);
void handle_startstream_msg			(server_ctx *ctx, uint8_t *data, uint16_t length);
void handle_stopstream_msg			(server_ctx *ctx, uint8_t *data, uint16_t length);
void handle_getresolution_msg		(server_ctx *ctx, uint8_t *data, uint16_t length);

#pragma pack(1)
typedef struct _mesg_header
{
	uint8_t opcode;
	uint16_t length;
} mesg_header;
#pragma pack(0)

#pragma pack(1)
typedef struct _frame_header
{
	uint16_t seq_number;
	uint16_t chunk_len;
	uint32_t total_len;
} frame_header;
#pragma pack(0)

// Handler des client messages
mesg_handler handlers[] =
{
	/* HELLO 			: */ handle_hello_msg,
	/* BYE 				: */ handle_bye_msg,	
	/* SET_FRAMERATE 	: */ handle_setframerate_msg,
	/* SET_RESOLUTION 	: */ handle_setresolution_msg,
	/* START_STREAM 	: */ handle_startstream_msg,
	/* STOP_STREAM 		: */ handle_stopstream_msg,
	/* GET_RESOLUTION 	: */ handle_getresolution_msg
};

void handle_hello_msg(server_ctx *ctx, uint8_t *data, uint16_t length)
{
	printf("Hello\n");
}

void handle_bye_msg	(server_ctx *ctx, uint8_t *data, uint16_t length)
{
	printf("Bye\n");
}

void handle_setframerate_msg(server_ctx *ctx, uint8_t *data, uint16_t length)
{
	client_mesg msg = {0};
	msg.byte[0] = SET_FRAMERATE;
	msg.byte[1] = *data;
	printf("Set Framerate %d fps\n", *data);
	chansendul(ctx->chan, msg.mesg);
}

void handle_setresolution_msg(server_ctx *ctx, uint8_t *data, uint16_t length)
{
	printf("Set Resolution\n");
}

void handle_startstream_msg(server_ctx *ctx, uint8_t *data, uint16_t length)
{
	printf("Start Stream\n");
	channbsendul(ctx->chan, START_STREAM);
}

void handle_stopstream_msg(server_ctx *ctx, uint8_t *data, uint16_t length)
{
	printf("Stop Stream\n");
	channbsendul(ctx->chan, STOP_STREAM);
}

void handle_getresolution_msg(server_ctx *ctx, uint8_t *data, uint16_t length)
{
	client_mesg msg = {0};
	msg.byte[0] = GET_RESOLUTION;
	printf("Get Resolution\n");
	chansendul(ctx->chan, msg.mesg);
}

// y'a certainement des optis a faire ici
void send_mesg(server_ctx *ctx, uint8_t opcode, uint8_t *data, uint16_t length)
{
	if(length > BUFFER_SIZE - sizeof(mesg_header))
	{
		printf("Error : message length too long\n");
		return;
	}
	
	mesg_header *hdr = (mesg_header*)send_buffer;
	hdr->opcode = opcode;
	hdr->length = length;
	
	if(length > 0)
	{
		memcpy(send_buffer + sizeof(mesg_header), data, length);
	}
	//printf("SEND OPCODE %x\n", opcode);
	
	// Decommenter pour debug des messages
	/*for(int bla=0; bla<sizeof(mesg_header) + length; bla++)
	{
		printf("%x ", send_buffer[bla]);
	}
	printf("\n");*/
	
	sendto(ctx->fd, send_buffer, sizeof(mesg_header) + length, 0, &ctx->from, ctx->fromlen);
}

void send_frame(server_ctx *ctx, uint8_t *ptr, uint32_t length)
{
	uint32_t bytes_remaining, nb_chunks;
	frame_header* hdr = (frame_header*)frame_buffer; 
	
	bytes_remaining = length;
	nb_chunks = (length / FRAME_CHUNK) + ((length % FRAME_CHUNK) != 0);
	
	hdr->total_len = length;
	for(int i=0; i<nb_chunks; i++)
	{
		hdr->seq_number = i;
		hdr->chunk_len = (i != nb_chunks-1) ? FRAME_CHUNK : bytes_remaining;
		bytes_remaining -= hdr->chunk_len;
		memcpy(frame_buffer + sizeof(frame_header), ptr + i*FRAME_CHUNK, hdr->chunk_len + sizeof(frame_header));
		printf("seq=%d;chnk_len=%d;bytes_remaining=%d\n", hdr->seq_number, hdr->chunk_len, bytes_remaining);
		send_mesg(ctx, FRAME, frame_buffer, hdr->chunk_len+sizeof(frame_header));
	}
	send_mesg(ctx, FRAME_DONE, NULL, 0);
}

void handle_mesg(server_ctx *ctx, uint8_t *mesg, uint16_t len)
{
	mesg_header* hdr;
	
	hdr = (mesg_header*)mesg;
	
	// check de l'opcode pour eviter de segfault, c'est quand meme chiant
	if((handlers == NULL) || (hdr->opcode < 0) || (hdr->opcode >= (sizeof(handlers) / sizeof(mesg_handler))))
	{
		printf("erreur opcode invalide ou handlers null\n");
		return;
	}	
	
	handlers[hdr->opcode](ctx, mesg+sizeof(mesg_header),hdr->length);
}

server_ctx* server_ctx_create(Channel *Chan)
{
	server_ctx *tmp = (server_ctx*)malloc(sizeof(server_ctx));
	if(tmp == NULL)
	{
		puts("Erreur malloc server_ctx");
		// TBD exit properly
		exit(EXIT_FAILURE);
	}
	tmp->fromlen = sizeof(struct sockaddr_in);
	tmp->chan = Chan; //alloue au dessus (cam_ctx)
	printf("chan (create server) = %x",tmp->chan);
	return tmp;
}

void server_ctx_destroy(server_ctx *ctx)
{
	free(ctx);
}

void server_ctx_start(server_ctx *ctx, uint16_t port)
{
	struct sockaddr_in me = {0};
	/* if((ctx->fd = netannounce(UDP, 0, port)) < 0){
		fprintf(stderr, "cannot announce on udp port %d: %s\n", port, strerror(errno));
		taskexitall(1);
	}	
	if(fdnoblock(ctx->fd) < 0){
		fprintf(stderr, "fdnoblock\n");
		taskexitall(1);
	}*/	
	
	ctx->fd = socket(AF_INET, SOCK_DGRAM, 0);
	me.sin_family = AF_INET;
	me.sin_addr.s_addr = INADDR_ANY;
	me.sin_port = htons(port);
	bind(ctx->fd, (struct sockaddr*)&me, sizeof(me));
}

void server_ctx_mainloop(server_ctx *ctx)
{
	int bytes;

	while(1)
	{
		fdwait(ctx->fd, 'r');
		printf("srv chan (servertask before recv) = %x\n",ctx->chan);
		bytes = recvfrom(ctx->fd, recv_buffer, sizeof(recv_buffer), 0, &ctx->from, &ctx->fromlen);
		printf("srv chan (servertask after recv) = %x\n",ctx->chan);
		printf("bytes read = %d\n", bytes);
		handle_mesg(ctx, recv_buffer, bytes);
		printf("srv chan (servertask after handle) = %x\n",ctx->chan);
	}
}

void servertask(void *arg)
{
	server_ctx *ctx = (server_ctx *)arg;
	server_ctx_start(ctx, PORT);	
	server_ctx_mainloop(ctx);
}
