/****************************************************************************
*
* Univerzitet u Novom Sadu, Fakultet tehnickih nauka
* Katedra za Računarsku Tehniku i Računarske Komunikacije
*
* -----------------------------------------------------
* Ispitni zadatak iz predmeta:
*
* PROGRAMSKA PODRSKA U TELEVIZIJI I OBRADI SLIKE
* -----------------------------------------------------
* Aplikacija za TV prijemnik
* -----------------------------------------------------
*
* \file init_controller.h
* \brief
* 	This module is used to read initial values from a file.
*
* @Author Mario Peric
*****************************************************************************/
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

/**
 * @brief Structure that defines init channel info
 */
typedef struct channel_st
{
	uint16_t videoPid;
	uint16_t audioPid;
	tStreamType videoType;
	tStreamType audioType;
} ChannelT;

/**
 * @brief Structure that defines init controller error
 */
typedef enum _InitControllerError
{
	IC_NO_ERROR = 0,
	IC_READ_ERROR,
	IC_PARSE_ERROR
} InitControllerError;

/**
 * @brief Reads init values from file
 *
 * @param  [in] file_name - path to file
 * @param  [out] freq - channel frequency
 * @param  [out] bandwidth - bandwidth
 * @param  [out] module - inital module
 * @param  [out] channel - initial channel values
 * @param  [out] program_no - initial program number
 *
 * @return init controller error code
 */
InitControllerError read_init_values(char *file_name, uint32_t *freq, uint32_t *bandwidth, t_Module *module, ChannelT *channel, uint16_t *program_no);

#endif //__INIT_CONTROLLER_H__
