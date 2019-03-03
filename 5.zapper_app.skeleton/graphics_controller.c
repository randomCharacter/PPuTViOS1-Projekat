#include "graphics_controller.h"

static timer_t timer_id;
static timer_t volume_timer_id;
static IDirectFBSurface *primary = NULL;
IDirectFB *dfb_interface = NULL;
static int32_t screen_width = 0;
static int32_t screen_height = 0;

static struct itimerspec timer_spec;
static struct itimerspec timer_spec_old;

static struct itimerspec volume_timer_spec;
static struct itimerspec volume_timer_spec_old;

static DFBSurfaceDescription surface_desc;

/* structure for timer specification */
struct sigevent signal_event;
struct sigevent volume_signal_event;

static bool draw = false;
static uint8_t thread_exit = 0;

IDirectFBFont *font_interface = NULL;
DFBFontDescription font_desc;
IDirectFBImageProvider *provider = NULL;

// Displaying data
static uint16_t sound_volume = 0;
static int16_t program_number;
static int16_t audio_pid;
static int16_t video_pid;
static bool teletext;
static char current_name[100];
static char next_name[100];
static bool radio = false;
static bool display_info = false;
static bool display_volume = false;

static pthread_t gc_thread;

static GraphicsControllerError clearInfo()
{
	if (radio)
	{
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xFF));
	}
	else
	{
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	}
	DFBCHECK(primary->FillRectangle(primary, screen_width / 16, 2.5 * screen_height / 4, 14 * screen_width / 16, 1.3 * screen_height / 4));

	return GC_NO_ERROR;
}

static GraphicsControllerError drawInfo()
{
	char text_string[100];

	/*  draw the frame */
	if (radio)
	{
		DFBCHECK(primary->SetColor(primary, 0x10, 0x80, 0x10, 0xFF));
	}
	else
	{
		DFBCHECK(primary->SetColor(primary, 0x10, 0x80, 0x10, 0xAA));
	}
	DFBCHECK(primary->FillRectangle(primary, screen_width / 16, 2.5 * screen_height / 4, 14 * screen_width / 16, 1.3 * screen_height / 4));
	if (radio)
	{
		DFBCHECK(primary->SetColor(primary, 0x10, 0x10, 0x10, 0xFF));
	}
	else
	{
		DFBCHECK(primary->SetColor(primary, 0x10, 0x10, 0x10, 0xAA));
	}
	DFBCHECK(primary->FillRectangle(primary, screen_width / 16 + FRAME_THICKNESS, 2.5 * screen_height / 4 + FRAME_THICKNESS, 14 * screen_width / 16 - 2 * FRAME_THICKNESS, 1.3 * screen_height / 4 - 2 * FRAME_THICKNESS));

	DFBCHECK(dfb_interface->CreateFont(dfb_interface, "/home/galois/fonts/DejaVuSans.ttf", &font_desc, &font_interface));
	DFBCHECK(primary->SetFont(primary, font_interface));

	font_desc.flags = DFDESC_HEIGHT;
	font_desc.height = FONT_HEIGHT;

	/* Channel */
	sprintf(text_string, "CHANNEL: %d", program_number);
	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, text_string, -1, screen_width / 16 + 2 * FRAME_THICKNESS, 2.5 * screen_height / 4 + FONT_HEIGHT / 2 + 4 * FRAME_THICKNESS, DSTF_LEFT));

	/* Video pid */
	if (!radio)
	{
		sprintf(text_string, "VIDEO PID: %d", video_pid);
		DFBCHECK(primary->DrawString(primary, text_string, -1, screen_width / 16 + 2 * FRAME_THICKNESS, 2.5 * screen_height / 4 + FONT_HEIGHT * 2 + 4 * FRAME_THICKNESS, DSTF_LEFT));
	}

	/* Audio pid */
	sprintf(text_string, "AUDIO PID: %d", audio_pid);
	DFBCHECK(primary->DrawString(primary, text_string, -1, screen_width / 16 + 2 * FRAME_THICKNESS, 2.5 * screen_height / 4 + FONT_HEIGHT * 3 + 4 * FRAME_THICKNESS, DSTF_LEFT));

	/* Teletext */
	sprintf(text_string, "TELETEXT: %s", teletext ? "YES" : "NO");
	DFBCHECK(primary->DrawString(primary, text_string, -1, screen_width / 16 + 2 * FRAME_THICKNESS, 2.5 * screen_height / 4 + FONT_HEIGHT * 4 + 4 * FRAME_THICKNESS, DSTF_LEFT));

	/* Channel info */
	if (!radio)
	{
		sprintf(text_string, "PLAYING: %s", current_name);
		DFBCHECK(primary->DrawString(primary, text_string, -1, screen_width / 16 + 2 * FRAME_THICKNESS, 2.5 * screen_height / 4 + FONT_HEIGHT * 5 + 4 * FRAME_THICKNESS, DSTF_LEFT));

		sprintf(text_string, "NEXT:    %s", next_name);
		DFBCHECK(primary->DrawString(primary, text_string, -1, screen_width / 16 + 2 * FRAME_THICKNESS, 2.5 * screen_height / 4 + FONT_HEIGHT * 6 + 4 * FRAME_THICKNESS, DSTF_LEFT));
	}

	return GC_NO_ERROR;
}

static GraphicsControllerError drawSoundVolume()
{
	int32_t ret;
	char img_name[50];
	IDirectFBSurface *logo_surface = NULL;
	int32_t logo_height, logo_width;

	/* get image name */
	sprintf(img_name, "volume_%u.png", sound_volume);

	/* create the image provider for the specified file */
	DFBCHECK(dfb_interface->CreateImageProvider(dfb_interface, img_name, &provider));

	/* get surface descriptor for the surface where the image will be rendered */
	DFBCHECK(provider->GetSurfaceDescription(provider, &surface_desc));

	/* create the surface for the image */
	DFBCHECK(dfb_interface->CreateSurface(dfb_interface, &surface_desc, &logo_surface));

	/* render the image to the surface */
	if (logo_surface)
	{
		DFBCHECK(provider->RenderTo(provider, logo_surface, NULL));
	}

	/* fetch the logo size and add (blit) it to the screen */
	DFBCHECK(logo_surface->GetSize(logo_surface, &logo_width, &logo_height));
	DFBCHECK(primary->Blit(primary,
						   /*source surface*/ logo_surface,
						   /*source region, NULL to blit the whole surface*/ NULL,
						   /*destination x coordinate of the upper left corner of the image*/ 50,
						   /*destination y coordinate of the upper left corner of the image*/ 50));

	if (provider != NULL)
	{
		provider->Release(provider);
	}
	if (logo_surface != NULL)
	{
		logo_surface->Release(logo_surface);
	}

	return GC_NO_ERROR;
}

static GraphicsControllerError clearSoundVolume()
{
	/* clear screen */
	if (radio)
	{
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xFF));
	}
	else
	{
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	}
	DFBCHECK(primary->FillRectangle(primary, 0, 0, 500, 500));

	return GC_NO_ERROR;
}

static GraphicsControllerError drawRadioScreen()
{
	DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xFF));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screen_width, screen_height));

	return GC_NO_ERROR;
}

static GraphicsControllerError clearRadioScreen()
{
	DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screen_width, screen_height));

	return GC_NO_ERROR;
}

static GraphicsControllerError drawEverything()
{
	int ret = 0;
	if (radio)
	{
		ret = ret || drawRadioScreen();
	}
	else
	{
		ret = ret || clearRadioScreen();
	}

	if (display_info)
	{
		ret = ret || drawInfo();
	}

	if (display_volume)
	{
		ret = ret || drawSoundVolume();
	}

	/* update screen */
	DFBCHECK(primary->Flip(primary, NULL, 0));

	return ret;
}

void *graphicsControllerTask()
{
	while (!thread_exit)
	{
		if (draw)
		{
			drawEverything();
			draw = false;
		}
	}
}

static void wipeScreen()
{
	int ret;

	display_info = false;

	draw = 1;

	/* stop the timer */
	memset(&timer_spec, 0, sizeof(timer_spec));
	ret = timer_settime(timer_id, 0, &timer_spec, &timer_spec_old);
	if (ret == -1)
	{
		printf("Error setting timer in %s!\n", __FUNCTION__);
	}
}

static void wipeVolumeScreen()
{
	int ret;

	display_volume = false;
	draw = 1;

	/* stop the timer */
	memset(&volume_timer_spec, 0, sizeof(volume_timer_spec));
	ret = timer_settime(volume_timer_id, 0, &volume_timer_spec, &volume_timer_spec_old);
	if (ret == -1)
	{
		printf("Error setting timer in %s!\n", __FUNCTION__);
	}
}

GraphicsControllerError graphicsControllerInit(int argc, char **argv)
{
	/* initialize DirectFB */
	int32_t ret;
	DFBCHECK(DirectFBInit(&argc, &argv));
	DFBCHECK(DirectFBCreate(&dfb_interface));
	DFBCHECK(dfb_interface->SetCooperativeLevel(dfb_interface, DFSCL_FULLSCREEN));

	/* create primary surface with double buffering enabled */
	surface_desc.flags = DSDESC_CAPS;
	surface_desc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK(dfb_interface->CreateSurface(dfb_interface, &surface_desc, &primary));

	/* fetch the screen size */
	DFBCHECK(primary->GetSize(primary, &screen_width, &screen_height));

	/* clear the screen before drawing anything (draw black full screen rectangle)*/

	DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xff));
	//DFBCHECK(primary->FillRectangle(primary, screen_width/3, screen_height/3, screen_width/3, screen_height/3));

	/* create timer */
	signal_event.sigev_notify = SIGEV_THREAD;		 /* tell the OS to notify you about timer by calling the specified function */
	signal_event.sigev_notify_function = wipeScreen; /* function to be called when timer runs out */
	signal_event.sigev_value.sival_ptr = NULL;		 /* thread arguments */
	signal_event.sigev_notify_attributes = NULL;	 /* thread attributes (e.g. thread stack size) - if NULL default attributes are applied */
	ret = timer_create(/*clock for time measuring*/ CLOCK_REALTIME,
					   /*timer settings*/ &signal_event,
					   /*where to store the ID of the newly created timer*/ &timer_id);

	volume_signal_event.sigev_notify = SIGEV_THREAD;			  /* tell the OS to notify you about timer by calling the specified function */
	volume_signal_event.sigev_notify_function = wipeVolumeScreen; /* function to be called when timer runs out */
	volume_signal_event.sigev_value.sival_ptr = NULL;			  /* thread arguments */
	volume_signal_event.sigev_notify_attributes = NULL;			  /* thread attributes (e.g. thread stack size) - if NULL default attributes are applied */
	ret = timer_create(/*clock for time measuring*/ CLOCK_REALTIME,
					   /*timer settings*/ &volume_signal_event,
					   /*where to store the ID of the newly created timer*/ &volume_timer_id);

	if (ret == -1)
	{
		printf("Error creating timer, abort!\n");
		primary->Release(primary);
		dfb_interface->Release(dfb_interface);
		DFBCHECK(font_interface->Release(font_interface));
		return GC_ERROR;
	}

	thread_exit = 0;
	if (pthread_create(&gc_thread, NULL, &graphicsControllerTask, NULL))
	{
		printf("Error creating input event task!\n");
		return GC_THREAD_ERROR;
	}

	return GC_NO_ERROR;
}

GraphicsControllerError graphicsControllerDeinit()
{
	thread_exit = 1;
	if (pthread_join(gc_thread, NULL))
	{
		printf("\n%s : ERROR pthread_join fail!\n", __FUNCTION__);
		return GC_THREAD_ERROR;
	}
	DFBCHECK(font_interface->Release(font_interface));
	DFBCHECK(primary->Release(primary));
	DFBCHECK(dfb_interface->Release(dfb_interface));
}

GraphicsControllerError drawChannelInfo(bool is_radio, int16_t program_no, int16_t audio, int16_t video, bool txt, char *current, char *next)
{
	int ret;

	display_info = true;
	radio = is_radio;

	updateChannelInfo(program_no, audio, video, txt, current, next);

	draw = 1;

	/* set the timer for clearing the screen */
	memset(&timer_spec, 0, sizeof(timer_spec));

	/* specify the timer timeout time */
	timer_spec.it_value.tv_sec = 3;
	timer_spec.it_value.tv_nsec = 0;

	/* set the new timer specs */
	ret = timer_settime(timer_id, 0, &timer_spec, &timer_spec_old);
	if (ret == -1)
	{
		printf("Error setting timer in %s!\n", __FUNCTION__);
		return GC_ERROR;
	}

	return GC_NO_ERROR;
}

GraphicsControllerError updateChannelInfo(int16_t program_no, int16_t audio, int16_t video, bool txt, char *current, char *next)
{
	program_number = program_no;
	audio_pid = audio;
	video_pid = video;
	teletext = txt;
	strcpy(current_name, current);
	strcpy(next_name, next);

	draw = 1;

	return GC_NO_ERROR;
}

GraphicsControllerError drawVolume(uint16_t volume)
{
	int ret;

	sound_volume = volume;
	display_volume = true;

	draw = 1;

	memset(&volume_timer_spec, 0, sizeof(timer_spec));

	/* specify the timer timeout time */
	volume_timer_spec.it_value.tv_sec = 3;
	volume_timer_spec.it_value.tv_nsec = 0;
	/* set the new timer specs */

	ret = timer_settime(volume_timer_id, 0, &volume_timer_spec, &volume_timer_spec_old);
	if (ret == -1)
	{
		printf("Error setting timer in %s!\n", __FUNCTION__);
		return GC_ERROR;
	}

	return GC_NO_ERROR;
}
