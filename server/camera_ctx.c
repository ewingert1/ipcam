#include "camera_ctx.h"

cam_ctx *cam_create()
{
	cam_ctx* tmp = (cam_ctx*)malloc(sizeof(cam_ctx));
	if(tmp == NULL)
	{
		fprintf(stderr, "Error creating cam context\n");
		exit(1);
	} 
	memset(tmp, 0, sizeof(cam_ctx));
	
	tmp->bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	tmp->bufferinfo.memory = V4L2_MEMORY_MMAP;
	
	tmp->chan = chancreate(sizeof(unsigned long), 1);
	printf("chan (create) = %x",tmp->chan);
	
	tmp->serv = server_ctx_create(tmp->chan);
	
	return tmp;
}

void cam_destroy(cam_ctx *ctx)
{
	for(int i=0; i<ctx->nb_buffers; ++i)
		munmap(ctx->buffers[i], ctx->buffer_size);
	free(ctx->buffers);
	chanfree(ctx->chan);
	free(ctx);
}

void cam_open_device(cam_ctx *ctx, const char* filename)
{
	// Ouverture du device en RDWR (WR pour le mmap)
    if((ctx->fd = open(filename, O_RDWR)) < 0){
        perror("open");
        exit(1);
    }
	if(ioctl(ctx->fd, VIDIOC_QUERYCAP, &ctx->cap) < 0){
    	perror("VIDIOC_QUERYCAP");
    	exit(1);
	}
	if(!(ctx->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
    	fprintf(stderr, "The device does not handle single-planar video capture.\n");
    	exit(1);
	}
}

void cam_close_device(cam_ctx *ctx)
{
	close(ctx->fd);
}

void cam_set_pixel_format(cam_ctx *ctx, int pix_format, int width, int height)
{
	ctx->width = width;
	ctx->height = height;
	
	ctx->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ctx->format.fmt.pix.pixelformat = pix_format;
	ctx->format.fmt.pix.width = width;
	ctx->format.fmt.pix.height = height;
	if(ioctl(ctx->fd, VIDIOC_S_FMT, &ctx->format) < 0){
    	perror("VIDIOC_S_FMT");
    	exit(1);
	}
} 

void cam_set_framerate(cam_ctx *ctx, int fps)
{
	ctx->fps = fps;
	
	ctx->s.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ctx->s.parm.capture.timeperframe.numerator = 1;
	ctx->s.parm.capture.timeperframe.denominator = fps;
	if(ioctl(ctx->fd,VIDIOC_S_PARM, &ctx->s) !=0) 
	{
     	puts("Failed to set frame rate ");
	}
} 

void cam_allocate_buffers(cam_ctx *ctx, int nb_buffers)
{
	int i = 0;
	struct v4l2_requestbuffers bufrequest;

	ctx->nb_buffers = nb_buffers;
	ctx->buffers = (void**)malloc(nb_buffers * sizeof(void*));
	if(!ctx->buffers)
	{
		printf("Erreur malloc du tableau de buffers eh\n");
	}

	bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	bufrequest.memory = V4L2_MEMORY_MMAP;
	bufrequest.count = nb_buffers;
	
	if(ioctl(ctx->fd, VIDIOC_REQBUFS, &bufrequest) < 0){
    	perror("VIDIOC_REQBUFS");
    	exit(1);
	}

	memset(&ctx->bufferinfo, 0, sizeof(ctx->bufferinfo));
	ctx->bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ctx->bufferinfo.memory = V4L2_MEMORY_MMAP;
	
	for(i=0; i<nb_buffers; ++i)
	{			
		ctx->bufferinfo.index = i;
		if(ioctl(ctx->fd, VIDIOC_QUERYBUF, &ctx->bufferinfo) < 0){
    		perror("VIDIOC_QUERYBUF");
    		exit(1);
		}		

		printf("buffer size = %d\nbuffer m offset = %d\n", ctx->bufferinfo.length, ctx->bufferinfo.m.offset);		
		ctx->buffers[i] = mmap(
    		NULL,
    		ctx->bufferinfo.length,
    		PROT_READ | PROT_WRITE,
    		MAP_SHARED,
    		ctx->fd,
    		ctx->bufferinfo.m.offset
		);

		if(ctx->buffers[i] == MAP_FAILED){
    		perror("mmap");
    		exit(1);
		}

		memset(ctx->buffers[i], 0, ctx->bufferinfo.length);	
	}
	ctx->buffer_size = ctx->bufferinfo.length;
}

int cam_queue_buffer(cam_ctx *ctx, int index)
{
	ctx->bufferinfo.index = index; 
		
	if(ioctl(ctx->fd, VIDIOC_QBUF, &ctx->bufferinfo) < 0){
    	perror("VIDIOC_QBUF");
    	return -1;
	}	
	return 0;
}

int cam_dequeue_buffer(cam_ctx *ctx, int index)
{
	ctx->bufferinfo.index = index; 

	if(ioctl(ctx->fd, VIDIOC_DQBUF, &ctx->bufferinfo) < 0){
    	perror("VIDIOC_DQBUF");
    	return -1;
	}
	
    return 0;
}

void cam_stream_on(cam_ctx *ctx)
{
	if(ioctl(ctx->fd, VIDIOC_STREAMON, &ctx->bufferinfo.type) < 0){
    	perror("VIDIOC_STREAMON");
    	exit(1);
	}
}

void cam_stream_off(cam_ctx *ctx)
{
	if(ioctl(ctx->fd, VIDIOC_STREAMOFF, &ctx->bufferinfo.type) < 0){
    	perror("VIDIOC_STREAMOFF");
    	exit(1);
	}
} 

void cam_start_serv(cam_ctx *ctx)
{
	taskcreate(servertask, (void*)ctx->serv, STACK);
}

void cam_handle_client_msg(cam_ctx *ctx, uint64_t client_msg)
{
	client_mesg *msg = (client_mesg*)&client_msg;
	uint8_t new_fps;
	uint16_t current_resolution[2], new_x, new_y;
	
	switch(msg->byte[0])
	{
	case SET_FRAMERATE:
		new_fps = msg->byte[1];
		printf("New framerate = %d\n", new_fps);
		// faire l'ioctl qui va bien
		break;
	case SET_RESOLUTION:
		new_x = msg->byte[1] + (msg->byte[2] << 8);
		new_y = msg->byte[3] + (msg->byte[4] << 8);
		printf("New resolution = %dx%d\n", new_x, new_y);
		// faire l'ioctl qui va bien
		break;
	case GET_RESOLUTION:
		current_resolution[0] = ctx->width;
		current_resolution[1] = ctx->height;
		// envoi de l'info
		send_mesg(ctx->serv, RESOLUTION, (uint8_t*)current_resolution, sizeof(current_resolution));
		break;
	default:
		break;
	}
	

}



