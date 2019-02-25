#ifndef __GRAPHICS_CONTROLLER_H__
#define __GRAPHICS_CONTROLLER_H__

#include <stdio.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <directfb.h>
#include <fcntl.h>
#include <signal.h>

#define FRAME_THICKNESS 20
#define FONT_HEIGHT 150

/* helper macro for error checking */
#define DFBCHECK(x...) { \
    DFBResult err = x; \
    if (err != DFB_OK) {  \
        fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
        DirectFBErrorFatal( #x, err ); \
    }  \
}

/**
 * @brief Structure that defines graphics controller error
 */
typedef enum _GraphicsControllerError
{
	GC_NO_ERROR = 0,
	GC_ERROR,
	GC_THREAD_ERROR
} GraphicsControllerError;

/*
 * @brief Initializes graphics controller module
 *
 * @return graphics cotroller error code
 */
GraphicsControllerError graphicsControllerInit();

/*
 * @brief Deinitializes graphics controller module
 *
 * @return graphics cotroller error code
 */
GraphicsControllerError graphicsControllerDeinit();

/*
 * @brief Draw channel number
 *
 * @return graphics cotroller error code
 */
GraphicsControllerError drawChannelNumber(int32_t);

void radioScreen();

#endif /* __REMOTE_CONTROLLER_H__ */