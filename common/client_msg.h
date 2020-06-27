#include <stdint.h>

// This file contains all messages sent by the camera client to the server

#define HELLO 				0
#define BYE 				1
#define SET_FRAMERATE		2
#define SET_RESOLUTION		3
#define START_STREAM		4
#define STOP_STREAM			5
#define GET_RESOLUTION		6

typedef union{
	uint64_t mesg;
	uint8_t byte[sizeof(uint64_t)];
} client_mesg;
