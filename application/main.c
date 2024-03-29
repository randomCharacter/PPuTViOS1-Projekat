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
* \file main.c
* \brief
* 	This module contains main TV app.
*
* @Author Mario Peric
*****************************************************************************/
#include "remote_controller.h"
#include "stream_controller.h"
#include "init_controller.h"
#include "volume_controller.h"
#include "graphics_controller.h"

static inline void textColor(int32_t attr, int32_t fg, int32_t bg)
{
	char command[13];

	/* command is the control command to the terminal */
	sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
	printf("%s", command);
}

/* macro function for error checking */
#define ERRORCHECK(x)                                                         \
	{                                                                         \
		if (x != 0)                                                           \
		{                                                                     \
			textColor(1, 1, 0);                                               \
			printf(" Error!\n File: %s \t Line: <%d>\n", __FILE__, __LINE__); \
			textColor(0, 7, 0);                                               \
			return -1;                                                        \
		}                                                                     \
	}

static void remoteControllerCallback(uint16_t code, uint16_t type, uint32_t value);
static pthread_cond_t deinit_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t deinit_mutex = PTHREAD_MUTEX_INITIALIZER;
static ChannelInfo channelInfo;

int main(int argc, char **argv)
{
	uint32_t freq;
	uint32_t bandwidth;
	t_Module module;
	ChannelT channel;
	uint16_t program_no;

	if (argc < 2)
	{
		printf("Usage: %s path/to/input\n", argv[0]);
		return -1;
	}

	if (read_init_values(argv[1], &freq, &bandwidth, &module, &channel, &program_no))
	{
		printf("Error reading inital values from %s\n", argv[1]);
		return -2;
	}

	/* initialize remote controller module */
	ERRORCHECK(remoteControllerInit());

	/* initialize graphics controller module */
	ERRORCHECK(graphicsControllerInit(argc, argv));

	/* register remote controller callback */
	ERRORCHECK(registerRemoteControllerCallback(remoteControllerCallback));

	/* initialize stream controller module */
	ERRORCHECK(streamControllerInit(freq, bandwidth, module, channel, program_no));

	/* wait for a EXIT remote controller key press event */
	pthread_mutex_lock(&deinit_mutex);
	if (ETIMEDOUT == pthread_cond_wait(&deinit_cond, &deinit_mutex))
	{
		printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
	}
	pthread_mutex_unlock(&deinit_mutex);

	/* unregister remote controller callback */
	ERRORCHECK(unregisterRemoteControllerCallback(remoteControllerCallback));

	/* deinitialize remote controller module */
	ERRORCHECK(remoteControllerDeinit());

	/* deinitialize stream controller module */
	ERRORCHECK(streamControllerDeinit());

	/* deinitialize graphics controller module */
	ERRORCHECK(graphicsControllerDeinit());

	return 0;
}

/**
 * @brief Remote controller callback
 *
 * @param [in]  code received IR code
 *              type type of event
 *              value value of event
 */
void remoteControllerCallback(uint16_t code, uint16_t type, uint32_t value)
{
	printf("Code: %d\n", code);
	switch (code)
	{
	case KEYCODE_INFO:
		getChannelInfo(&channelInfo);
		drawChannelInfo(channelInfo.isRadio, channelInfo.programNumber, channelInfo.audioPid, channelInfo.videoPid, channelInfo.teletext, channelInfo.currentInfo, channelInfo.nextInfo);
		printf("\nInfo pressed\n");
		if (getChannelInfo(&channelInfo) == SC_NO_ERROR)
		{
			printf("\n********************* Channel info *********************\n");
			printf("Program number: %d\n", channelInfo.programNumber);
			printf("Audio pid: %d\n", channelInfo.audioPid);
			printf("Video pid: %d\n", channelInfo.videoPid);
			printf("**********************************************************\n");
		}
		break;
	case KEYCODE_P_PLUS:
		printf("\nCH+ pressed\n");
		channelUp();
		break;
	case KEYCODE_P_MINUS:
		printf("\nCH- pressed\n");
		channelDown();
		break;
	case KEYCODE_EXIT:
		printf("\nExit pressed\n");
		pthread_mutex_lock(&deinit_mutex);
		pthread_cond_signal(&deinit_cond);
		pthread_mutex_unlock(&deinit_mutex);
		break;
	case KEYCODE_0:
		SetChannel(0);
		break;
	case KEYCODE_1:
		SetChannel(1);
		break;
	case KEYCODE_2:
		SetChannel(2);
		break;
	case KEYCODE_3:
		SetChannel(3);
		break;
	case KEYCODE_4:
		SetChannel(4);
		break;
	case KEYCODE_5:
		SetChannel(5);
		break;
	case KEYCODE_6:
		SetChannel(6);
		break;
	case KEYCODE_7:
		SetChannel(7);
		break;
	case KEYCODE_8:
		SetChannel(8);
		break;
	case KEYCODE_9:
		SetChannel(9);
		break;
	case KEYCODE_V_PLUS:
		printf("\nV+ pressed\n");
		volumeUp();
		break;
	case KEYCODE_V_MINUS:
		printf("\nV- pressed\n");
		volumeDown();
		break;
	case KEYCODE_MUTE:
		muteVolume();
		break;
	default:
		printf("\nPress P+, P-, info or exit! \n\n");
	}
}
