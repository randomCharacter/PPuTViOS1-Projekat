#include "graphics_controller.h"

static timer_t timerId;
static timer_t volumeTimerId;
static IDirectFBSurface *primary = NULL;
IDirectFB *dfbInterface = NULL;
static int32_t screenWidth = 0;
static int32_t screenHeight = 0;

static struct itimerspec timerSpec;
static struct itimerspec timerSpecOld;

static struct itimerspec volumeTimerSpec;
static struct itimerspec volumeTimerSpecOld;

static DFBSurfaceDescription surfaceDesc;

/* structure for timer specification */
struct sigevent signalEvent;
struct sigevent volumeSignalEvent;

static bool draw = false;
static uint8_t threadExit = 0;

IDirectFBFont *fontInterface = NULL;
DFBFontDescription fontDesc;
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

static pthread_t gcThread;

static GraphicsControllerError clearInfo() {
	if (radio) {
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xFF));
	} else {
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	}
	DFBCHECK(primary->FillRectangle(primary, screenWidth/16, 2.5 * screenHeight/4, 14 * screenWidth/16, 1.3 * screenHeight/4));

	return GC_NO_ERROR;
}

static GraphicsControllerError drawInfo() {
	char keycodeString[100];

	/*  draw the frame */
	if (radio) {
		DFBCHECK(primary->SetColor(primary, 0x10, 0x80, 0x10, 0xFF));
	} else {
		DFBCHECK(primary->SetColor(primary, 0x10, 0x80, 0x10, 0xAA));
	}
	DFBCHECK(primary->FillRectangle(primary, screenWidth/16, 2.5 * screenHeight/4, 14 * screenWidth/16, 1.3 * screenHeight/4));
	if (radio) {
		DFBCHECK(primary->SetColor(primary, 0x10, 0x10, 0x10, 0xFF));
	} else {
		DFBCHECK(primary->SetColor(primary, 0x10, 0x10, 0x10, 0xAA));
	}
	DFBCHECK(primary->FillRectangle(primary, screenWidth/16+FRAME_THICKNESS, 2.5 * screenHeight/4+FRAME_THICKNESS, 14 * screenWidth/16-2*FRAME_THICKNESS, 1.3 * screenHeight/4-2*FRAME_THICKNESS));

	DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));

	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = FONT_HEIGHT;

	/* Channel */
	sprintf(keycodeString,"CHANNEL: %d",program_number);
	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/16 + 2*FRAME_THICKNESS, 2.5 * screenHeight/4+FONT_HEIGHT/2 + 4 * FRAME_THICKNESS, DSTF_LEFT));

	/* Video pid */
	if (!radio) {
		sprintf(keycodeString,"VIDEO PID: %d", video_pid);
		DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/16 + 2*FRAME_THICKNESS, 2.5 * screenHeight/4+FONT_HEIGHT * 2 + 4 * FRAME_THICKNESS, DSTF_LEFT));
	}

	/* Audio pid */
	sprintf(keycodeString,"AUDIO PID: %d", audio_pid);
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/16 + 2*FRAME_THICKNESS, 2.5 * screenHeight/4+FONT_HEIGHT * 3 + 4 * FRAME_THICKNESS, DSTF_LEFT));

	/* Teletext */
	sprintf(keycodeString,"TELETEXT: %s", teletext? "YES" : "NO");
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/16 + 2*FRAME_THICKNESS, 2.5 * screenHeight/4+FONT_HEIGHT * 4 + 4 * FRAME_THICKNESS, DSTF_LEFT));

	/* Channel info */
	if (!radio) {
		sprintf(keycodeString,"PLAYING: %s", current_name);
		DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/16 + 2*FRAME_THICKNESS, 2.5 * screenHeight/4+FONT_HEIGHT * 5 + 4 * FRAME_THICKNESS, DSTF_LEFT));

		sprintf(keycodeString,"NEXT:    %s", next_name);
		DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/16 + 2*FRAME_THICKNESS, 2.5 * screenHeight/4+FONT_HEIGHT * 6 + 4 * FRAME_THICKNESS, DSTF_LEFT));
	}

	return GC_NO_ERROR;
}

static GraphicsControllerError drawSoundVolume() {
	int32_t ret;
	char img_name[50];
	IDirectFBSurface *logoSurface = NULL;
	int32_t logoHeight, logoWidth;

	/* get image name */
	sprintf(img_name, "volume_%u.png", sound_volume);

	/* create the image provider for the specified file */
	DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, img_name, &provider));

	/* get surface descriptor for the surface where the image will be rendered */
	DFBCHECK(provider->GetSurfaceDescription(provider, &surfaceDesc));

	/* create the surface for the image */
	DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &logoSurface));

	/* render the image to the surface */
	if (logoSurface) {
		DFBCHECK(provider->RenderTo(provider, logoSurface, NULL));
	}
	

	/* fetch the logo size and add (blit) it to the screen */
	DFBCHECK(logoSurface->GetSize(logoSurface, &logoWidth, &logoHeight));
	DFBCHECK(primary->Blit(primary,
						   /*source surface*/ logoSurface,
						   /*source region, NULL to blit the whole surface*/ NULL,
						   /*destination x coordinate of the upper left corner of the image*/50,
						   /*destination y coordinate of the upper left corner of the image*/50));

	if (provider != NULL) {
		provider->Release(provider);
	}
	if (logoSurface != NULL) {
		logoSurface->Release(logoSurface);
	}

	return GC_NO_ERROR;
}

static GraphicsControllerError clearSoundVolume() {
	/* clear screen */
	if (radio) {
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xFF));
	} else {
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	}
	DFBCHECK(primary->FillRectangle(primary, 0, 0, 500, 500));

	return GC_NO_ERROR;
}

static GraphicsControllerError drawRadioScreen() {
	DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xFF));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));

	return GC_NO_ERROR;
}

static GraphicsControllerError clearRadioScreen() {
	DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));

	return GC_NO_ERROR;
}

static GraphicsControllerError drawEverything() {
	int ret = 0;
	if (radio) {
		ret = ret || drawRadioScreen();
	} else {
		ret = ret || clearRadioScreen();
	}

	if (display_info) {
		ret = ret || drawInfo();
	}

	if (display_volume) {
		ret = ret || drawSoundVolume();
	}

	/* update screen */
	DFBCHECK(primary->Flip(primary, NULL, 0));

	return ret;
}

void* graphicsControllerTask()
{
	while(!threadExit)
	{
		if (draw)
		{
			drawEverything();
			draw = false;
		}
	}
}

static void wipeScreen() {
	int ret;

	display_info = false;

	draw = 1;

	/* stop the timer */
	memset(&timerSpec,0,sizeof(timerSpec));
	ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
	if(ret == -1){
		printf("Error setting timer in %s!\n", __FUNCTION__);
	}
}

static void wipeVolumeScreen() {
	int ret;

	display_volume = false;
	draw = 1;

	/* stop the timer */
	memset(&volumeTimerSpec,0,sizeof(volumeTimerSpec));
	ret = timer_settime(volumeTimerId,0,&volumeTimerSpec,&volumeTimerSpecOld);
	if(ret == -1){
		printf("Error setting timer in %s!\n", __FUNCTION__);
	}
}

void radioScreen(int16_t program_number, int16_t audio_pid, int16_t video_pid, bool teletext, char* current_name, char* next_name) {
	drawChannelInfo(true, program_number, audio_pid, video_pid, teletext, current_name, next_name);
}

void videoScreen(int16_t program_number, int16_t audio_pid, int16_t video_pid, bool teletext, char* current_name, char* next_name) {
	drawChannelInfo(false, program_number, audio_pid, video_pid, teletext, current_name, next_name);
}

GraphicsControllerError graphicsControllerInit(int argc, char** argv) {
	/* initialize DirectFB */
	int32_t ret;
	DFBCHECK(DirectFBInit(&argc, &argv));
	DFBCHECK(DirectFBCreate(&dfbInterface));
	DFBCHECK(dfbInterface->SetCooperativeLevel(dfbInterface, DFSCL_FULLSCREEN));


	/* create primary surface with double buffering enabled */
	surfaceDesc.flags = DSDESC_CAPS;
	surfaceDesc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK (dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &primary));

	/* fetch the screen size */
	DFBCHECK (primary->GetSize(primary, &screenWidth, &screenHeight));


	/* clear the screen before drawing anything (draw black full screen rectangle)*/

	DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xff));
	//DFBCHECK(primary->FillRectangle(primary, screenWidth/3, screenHeight/3, screenWidth/3, screenHeight/3));

	/* create timer */
	signalEvent.sigev_notify = SIGEV_THREAD; /* tell the OS to notify you about timer by calling the specified function */
	signalEvent.sigev_notify_function = wipeScreen; /* function to be called when timer runs out */
	signalEvent.sigev_value.sival_ptr = NULL; /* thread arguments */
	signalEvent.sigev_notify_attributes = NULL; /* thread attributes (e.g. thread stack size) - if NULL default attributes are applied */
	ret = timer_create(/*clock for time measuring*/CLOCK_REALTIME,
					   /*timer settings*/&signalEvent,
					   /*where to store the ID of the newly created timer*/&timerId);

	volumeSignalEvent.sigev_notify = SIGEV_THREAD; /* tell the OS to notify you about timer by calling the specified function */
	volumeSignalEvent.sigev_notify_function = wipeVolumeScreen; /* function to be called when timer runs out */
	volumeSignalEvent.sigev_value.sival_ptr = NULL; /* thread arguments */
	volumeSignalEvent.sigev_notify_attributes = NULL; /* thread attributes (e.g. thread stack size) - if NULL default attributes are applied */
	ret = timer_create(/*clock for time measuring*/CLOCK_REALTIME,
					   /*timer settings*/&volumeSignalEvent,
					   /*where to store the ID of the newly created timer*/&volumeTimerId);

	if(ret == -1){
		printf("Error creating timer, abort!\n");
		primary->Release(primary);
		dfbInterface->Release(dfbInterface);
		DFBCHECK(fontInterface->Release(fontInterface));
		return GC_ERROR;
	}

	threadExit = 0;
	if (pthread_create(&gcThread, NULL, &graphicsControllerTask, NULL))
	{
		printf("Error creating input event task!\n");
		return GC_THREAD_ERROR;
	}

	return GC_NO_ERROR;
}

GraphicsControllerError graphicsControllerDeinit() {
	threadExit = 1;
	if (pthread_join(gcThread, NULL))
	{
		printf("\n%s : ERROR pthread_join fail!\n", __FUNCTION__);
		return GC_THREAD_ERROR;
	}
	DFBCHECK(fontInterface->Release(fontInterface));
	DFBCHECK(primary->Release(primary));
	DFBCHECK(dfbInterface->Release(dfbInterface));
}

GraphicsControllerError drawChannelInfo(bool is_radio, int16_t program_no, int16_t audio, int16_t video, bool txt, char* current, char* next) {
	int ret;

	display_info = true;
	radio = is_radio;

	updateChannelInfo(program_no, audio, video, txt, current, next);

	draw = 1;

	/* set the timer for clearing the screen */
	memset(&timerSpec,0,sizeof(timerSpec));

	/* specify the timer timeout time */
	timerSpec.it_value.tv_sec = 3;
	timerSpec.it_value.tv_nsec = 0;

	/* set the new timer specs */
	ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
	if(ret == -1){
		printf("Error setting timer in %s!\n", __FUNCTION__);
		return GC_ERROR;
	}

	return GC_NO_ERROR;
}

GraphicsControllerError updateChannelInfo(int16_t program_no, int16_t audio, int16_t video, bool txt, char* current, char* next) {
	program_number = program_no;
	audio_pid = audio;
	video_pid = video;
	teletext = txt;
	strcpy(current_name, current);
	strcpy(next_name, next);

	draw = 1;

	return GC_NO_ERROR;
}

GraphicsControllerError drawVolume(uint16_t volume) {
	int ret;

	sound_volume = volume;
	display_volume = true;

	draw = 1;

	memset(&volumeTimerSpec,0,sizeof(timerSpec));

	/* specify the timer timeout time */
	volumeTimerSpec.it_value.tv_sec = 3;
	volumeTimerSpec.it_value.tv_nsec = 0;
	/* set the new timer specs */

	ret = timer_settime(volumeTimerId,0,&volumeTimerSpec,&volumeTimerSpecOld);
	if(ret == -1){
		printf("Error setting timer in %s!\n", __FUNCTION__);
		return GC_ERROR;
	}

	return GC_NO_ERROR;
}
