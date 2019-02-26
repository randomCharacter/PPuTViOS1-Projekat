#ifndef __VOLUME_CONTROLLER_H__
#define __VOLUME_CONTROLLER_H__

#include <stdio.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>

#define VOLUME_LEVEL 100000000
#define INIT_VOLUME 5

typedef enum _VolumeControllerError
{
	VC_NO_ERROR = 0,
	VC_ERROR
}VolumeControllerError;

VolumeControllerError volumeControllerInit(uint32_t playerHandle);

VolumeControllerError volumeUp();

VolumeControllerError volumeDown();

VolumeControllerError getVolume(uint32_t *volume);

VolumeControllerError muteVolume();

#endif