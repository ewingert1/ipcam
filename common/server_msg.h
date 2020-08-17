#include <stdint.h>

// This file contains all messages sent by the server to the client

#define HELLO 			0
#define BYE 			1
#define FRAME			2
#define FRAME_DONE		3
#define RESOLUTION		4


#pragma pack(1)
typedef struct _mesg_header
{
	uint8_t opcode;
	uint64_t length;
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
