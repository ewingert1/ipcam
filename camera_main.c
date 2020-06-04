#include "camera_ctx.h"

int running = 1;

void irq_handler(int dummy)
{
	running = 0;
}

void taskmain(int argc, char **argv)
{
	int buf, nb_buf;
	unsigned long gonogo = 0;
	struct timeval tv1,tv2;	
	cam_ctx *ctx;

	buf = 0;
	nb_buf = 3;
	
	//signal(SIGINT, irq_handler);
	
	ctx = cam_ctx_create();
	cam_ctx_start_serv(ctx);
	// Laisser du temps au serveur pour s'init et attendre
	taskyield();
	
	// TBD : conf
	cam_ctx_open_device(ctx, "/dev/video3");
	cam_ctx_set_pixel_format(ctx, V4L2_PIX_FMT_YUYV, 640, 480);
	cam_ctx_set_framerate(ctx, 20);	
	cam_ctx_allocate_buffers(ctx, nb_buf);
		
	while(running){
		gonogo = chanrecvul(ctx->chan);
		if(gonogo != START_STREAM)
		{
			// Si le message n'est pas START STREAM : traiter l'eventuelle requete et continue
			printf("gonogo = %x\n", gonogo);
			cam_ctx_handle_client_msg(ctx, gonogo);
			continue;
		}
		
		
		
		cam_ctx_stream_on(ctx);
		for(buf=0; buf<nb_buf; ++buf)
			cam_ctx_queue_buffer(ctx, buf);
			
		gettimeofday(&tv1, NULL);		
		buf = 0;
		while(running){
    		if(cam_ctx_dequeue_buffer(ctx, buf%nb_buf) == 0)
    		{
    			gettimeofday(&tv2, NULL);
    			printf("fps = %f\n", 1000000./((tv2.tv_sec*1000000 + tv2.tv_usec) - (tv1.tv_sec*1000000 + tv1.tv_usec)));
    			gettimeofday(&tv1, NULL);    	
    			// handle ctx->buffers[buf]
    			send_frame(ctx->serv, ctx->buffers[buf], ctx->buffer_size);    	
    			getchar();
    		}    	
    		cam_ctx_queue_buffer(ctx, buf%nb_buf);
    		buf++;
    		// check for stop stream (non blocking)
    		gonogo = channbrecvul(ctx->chan);
			if(gonogo != STOP_STREAM) 
    		{
    			// on a envoye une frame => on peut traiter une requete et reprendre
    			cam_ctx_handle_client_msg(ctx, gonogo);	
    		}
    		else
    		{
    			// C'est une requete de stop stream => on casse et on attend la prochaine requete
    			break;
    		}
    		
    		taskdelay(1);
		}
		puts("On a stopped le stream");
		cam_ctx_stream_off(ctx);
	}
	
	cam_ctx_close_device(ctx);
	cam_ctx_destroy(ctx);	
	
	taskexitall(0);
}
