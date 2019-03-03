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

#include "graphics_controller.h"

/* Change of volume per level */
#define VOLUME_LEVEL 100000000
/* Initial level of volume */
#define INIT_VOLUME 5

/**
 * @brief Structure that defines volume controller error
 */
typedef enum _VolumeControllerError
{
	VC_NO_ERROR = 0,
	VC_ERROR
} VolumeControllerError;

/**
 * @brief Initializes volume controller module
 *
 * param [in] playerHandle - active player handle
 *
 * @return stream controller error code
 */
VolumeControllerError volumeControllerInit(uint32_t playerHandle);

/**
 * @brief Volume up
 *
 * @return volume controller error
 */
VolumeControllerError volumeUp();

/**
 * @brief Volume down
 *
 * @return volume controller error
 */
VolumeControllerError volumeDown();

/**
 * @brief Returns current value of volume
 *
 * @param [out] volume - current level of volume
 *
 * @return volume controller error
 */
VolumeControllerError getVolume(uint16_t *volume);

/**
 * @brief Mutes sound
 *
 * @return volume controller error
 */
VolumeControllerError muteVolume();

#endif
