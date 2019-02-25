#ifndef __RAMOTE_CONTROLLER_H__
#define __REMOTE_CONTROLLER_H__

#include <stdio.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>

#define KEYCODE_EXIT 102
#define KEYCODE_P_PLUS 62
#define KEYCODE_P_MINUS 61
#define KEYCODE_INFO 358

/* input event values for 'EV_KEY' type */
#define EV_VALUE_RELEASE    0
#define EV_VALUE_KEYPRESS   1
#define EV_VALUE_AUTOREPEAT 2

/**
 * @brief Structure that defines remote controller error
 */
typedef enum _RemoteControllerError
{
	RC_NO_ERROR = 0,
	RC_ERROR,
	RC_THREAD_ERROR
}RemoteControllerError;

/**
 * @brief Remote controller callback
 */
typedef void(*RemoteControllerCallback)(uint16_t code, uint16_t type, uint32_t value);

/*
 * @brief Initializes remote controller module
 *
 * @return remote cotroller error code
 */
RemoteControllerError remoteControllerInit();

/*
 * @brief Deinitializes remote controller module
 *
 * @return remote cotroller error code
 */
RemoteControllerError remoteControllerDeinit();

/*
 * @brief Registers remote controller callback
 *
 * @param  [in] remote controller callback - pointer to remote controller callback function
 * @return remote controller error code
 */
RemoteControllerError registerRemoteControllerCallback(RemoteControllerCallback remoteControllerCallback);

/*
 * @brief Unregisters remote controller callback
 *
 * @param  [in] remoteControllerCallback - pointer to remote controller callback function
 * @return remote controller error code
 */
RemoteControllerError unregisterRemoteControllerCallback(RemoteControllerCallback remoteControllerCallback);

#endif /* __REMOTE_CONTROLLER_H__ */

