#ifndef __INIT_CONTROLLER_H__
#define __INIT_CONTROLLER_H__

#include <stdio.h>
#include <ctype.h>
#include "tdp_api.h"


#define INIT_FILE_NAME "config.txt"
#define INIT_KEY_SIZE 51

#define KEY_FREQ "freq:"
#define KEY_BANDWITH "bandwidth:"
#define KEY_MODULE "module:"
#define KEY_PROGRAM_NO "program:"
#define KEY_VIDEO_PID "channel_video_pid:"
#define KEY_AUDIO_PID "channel_audio_pid:"
#define KEY_VIDEO_TYPE "channel_video_type:"
#define KEY_AUDIO_TYPE "channel_audio_type:"

typedef struct channel_st {
	uint16_t video_pid;
	uint16_t audio_pid;
	tStreamType video_type;
	tStreamType audio_type;
} ChannelT;

typedef enum _InitControllerError
{
	IC_NO_ERROR = 0,
	IC_READ_ERROR,
	IC_PARSE_ERROR
} InitControllerError;


InitControllerError read_init_values(char *file_name, uint32_t *freq, uint32_t *bandwidth, t_Module *module, ChannelT *channel, uint16_t *program_no);

#endif //__INIT_CONTROLLER_H__
