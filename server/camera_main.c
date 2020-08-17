#include "camera_ctx.h"

int running = 1;

void irq_handler(int dummy)
{
	running = 0;
}

void taskmain(int argc, char **argv)
{
	int buf, nb_buf;
	unsigned long status = 0;
	struct timeval tv1,tv2;	
	cam_ctx *ctx;

	buf = 0;
	nb_buf = 1;
	
	//signal(SIGINT, irq_handler);
	
	ctx = cam_create();
	cam_start_serv(ctx);
	// Laisser du temps au serveur pour s'init et attendre
	taskdelay(1);
	//taskyield();
	
	// TBD : conf
	cam_open_device(ctx, "/dev/video4");
	cam_set_pixel_format(ctx, V4L2_PIX_FMT_YUYV, 640, 480);
	cam_set_framerate(ctx, 10);	
	cam_allocate_buffers(ctx, nb_buf);
		
	while(running){
		// wait for start stream (blocking)
		status = chanrecvul(ctx->chan);
		if(status != START_STREAM)
		{
			// Si le message n'est pas START STREAM : traiter l'eventuelle requete et continue
			printf("status = %x\n", status);
			cam_handle_client_msg(ctx, status);
			continue;
		}
		
		cam_stream_on(ctx);
		for(buf=0; buf<nb_buf; ++buf)
			cam_queue_buffer(ctx, buf);
			
		gettimeofday(&tv1, NULL);		
		buf = 0;
		while(running){
			if(cam_dequeue_buffer(ctx, buf%nb_buf) == 0)
    		{
    			gettimeofday(&tv2, NULL);
    			printf("fps = %f\n", 1000000./((tv2.tv_sec*1000000 + tv2.tv_usec) - (tv1.tv_sec*1000000 + tv1.tv_usec)));
    			gettimeofday(&tv1, NULL);    	
    			// handle ctx->buffers[buf]
    			send_frame(ctx->serv, ctx->buffers[buf%nb_buf], ctx->buffer_size);    	
    			//getchar();
    		}    	
    		cam_queue_buffer(ctx, buf%nb_buf);
    		buf++;
    		// check for stop stream (non blocking)
    		status = channbrecvul(ctx->chan);
    		// pas de data dans le channel (cas le plus courant) => on yield et on continue
    		if(status == 0)
    		{
    			taskdelay(1);
    			continue;
    		}
    		// un stop stream dans le channel : on break
			if(status == STOP_STREAM) 
    		{
    			break;
    		}

			// si on arrive ici, on a recu une autre demande (get ou set pour la camera)
			// on la traite et on continue
    		cam_handle_client_msg(ctx, status);	
		
    		taskdelay(1);
		}
		
		puts("stream stopped");
		cam_stream_off(ctx);
	}
	
	cam_close_device(ctx);
	cam_destroy(ctx);	
	
	taskexitall(0);
}
