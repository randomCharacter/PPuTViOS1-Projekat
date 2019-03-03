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
#define KEYCODE_0 11
#define KEYCODE_1 2
#define KEYCODE_2 3
#define KEYCODE_3 4
#define KEYCODE_4 5
#define KEYCODE_5 6
#define KEYCODE_6 7
#define KEYCODE_7 8
#define KEYCODE_8 9
#define KEYCODE_9 10
#define KEYCODE_MUTE 60
#define KEYCODE_V_PLUS 63
#define KEYCODE_V_MINUS 64

/* input event values for 'EV_KEY' type */
#define EV_VALUE_RELEASE 0
#define EV_VALUE_KEYPRESS 1
#define EV_VALUE_AUTOREPEAT 2

/**
 * @brief Structure that defines remote controller error
 */
typedef enum _RemoteControllerError
{
	RC_NO_ERROR = 0,
	RC_ERROR,
	RC_THREAD_ERROR
} RemoteControllerError;

/**
 * @brief Remote controller callback
 */
typedef void (*RemoteControllerCallback)(uint16_t code, uint16_t type, uint32_t value);

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
 * @param  [in] remote_controller_callback - pointer to remote controller callback function
 *
 * @return remote controller error code
 */
RemoteControllerError registerRemoteControllerCallback(RemoteControllerCallback remote_controller_callback);

/*
 * @brief Unregisters remote controller callback
 *
 * @param  [in] remote_controller_callback - pointer to remote controller callback function
 *
 * @return remote controller error code
 */
RemoteControllerError unregisterRemoteControllerCallback(RemoteControllerCallback remote_controller_callback);

#endif /* __REMOTE_CONTROLLER_H__ */
