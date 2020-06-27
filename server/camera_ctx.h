#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>

#include "server_ctx.h"

typedef struct camera_ctx{

	int fd;
	int width;
	int height;
	int fps;
	int buffer_size;
	int nb_buffers;
	void** buffers;

	struct v4l2_buffer bufferinfo;
	struct v4l2_capability cap;
	struct v4l2_streamparm s;
	struct v4l2_format format;
	
	Channel *chan;
	server_ctx *serv;

}cam_ctx;

cam_ctx *cam_create();
void cam_destroy(cam_ctx *ctx);
void cam_open_device(cam_ctx *ctx, const char* filename);
void cam_close_device(cam_ctx *ctx);
void cam_set_pixel_format(cam_ctx *ctx, int pix_format, int width, int height); 
void cam_set_framerate(cam_ctx *ctx, int fps);
void cam_allocate_buffers(cam_ctx *ctx, int nb_buffers);
int cam_queue_buffer(cam_ctx *ctx, int index);
int cam_dequeue_buffer(cam_ctx *ctx, int index);
void cam_stream_on(cam_ctx *ctx);
void cam_stream_off(cam_ctx *ctx);
void cam_start_serv(cam_ctx *ctx);
void cam_handle_client_msg(cam_ctx *ctx, uint64_t client_msg);
