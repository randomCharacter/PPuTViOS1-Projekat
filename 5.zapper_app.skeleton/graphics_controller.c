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

static int radio = 0;

/* structure for timer specification */
struct sigevent signalEvent;
struct sigevent volumeSignalEvent;

static void wipeScreen() {
	int32_t ret;

	/* clear screen */
	if (radio) {
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xFF));
	} else {
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	}
	DFBCHECK(primary->FillRectangle(primary, screenWidth/16, 2.5 * screenHeight/4, 14 * screenWidth/16, 1.3 * screenHeight/4));

	/* update screen */
	DFBCHECK(primary->Flip(primary, NULL, 0));

	/* clear screen */
	if (radio) {
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xFF));
	} else {
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	}
	DFBCHECK(primary->FillRectangle(primary, screenWidth/16, 2.5 * screenHeight/4, 14 * screenWidth/16, 1.3 * screenHeight/4));

	/* stop the timer */
	memset(&timerSpec,0,sizeof(timerSpec));
	ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
	if(ret == -1){
		printf("Error setting timer in %s!\n", __FUNCTION__);
	}
}

static void wipeVolumeScreen() {
	int32_t ret;

	/* clear screen */
	if (radio) {
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xFF));
	} else {
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	}
	DFBCHECK(primary->FillRectangle(primary, 0, 0, 500, 500));

	/* update screen */
	DFBCHECK(primary->Flip(primary, NULL, 0));

	/* clear screen */
	if (radio) {
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xFF));
	} else {
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	}
	DFBCHECK(primary->FillRectangle(primary, 0, 0, 500, 500));

	/* stop the timer */
	memset(&volumeTimerSpec,0,sizeof(volumeTimerSpec));
	ret = timer_settime(volumeTimerId,0,&volumeTimerSpec,&volumeTimerSpecOld);
	if(ret == -1){
		printf("Error setting timer in %s!\n", __FUNCTION__);
	}
}

void radioScreen(int32_t channel_no) {
	int32_t ret;

	/* clear screen */
	DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xFF));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));

	/* update screen */
	DFBCHECK(primary->Flip(primary, NULL, 0));

	/* clear screen */
	DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xFF));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));

	radio = 1;
	drawChannelNumber(channel_no);
}

void videoScreen(int32_t channel_no) {
	int32_t ret;

	/* clear screen */
	DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));

	/* update screen */
	DFBCHECK(primary->Flip(primary, NULL, 0));

	/* clear screen */
	DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));

	radio = 0;
	drawChannelNumber(channel_no);

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
	DFBCHECK(primary->FillRectangle(primary, screenWidth/3, screenHeight/3, screenWidth/3, screenHeight/3));

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

		return GC_ERROR;
	}

	return GC_NO_ERROR;
}

GraphicsControllerError drawChannelNumber(int32_t keycode) {
	int32_t ret;
	IDirectFBFont *fontInterface = NULL;
	DFBFontDescription fontDesc;
	char keycodeString[4];

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


	/* draw keycode */

	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = FONT_HEIGHT;

	DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));

	/* generate keycode string */
	sprintf(keycodeString,"%d",keycode);

	/* draw the string */
	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/16 + 2*FRAME_THICKNESS, 2.5 * screenHeight/4+FONT_HEIGHT/2 + 4 * FRAME_THICKNESS, DSTF_LEFT));


	/* update screen */
	DFBCHECK(primary->Flip(primary, NULL, 0));
	// REPEAT
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


	/* draw keycode */

	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = FONT_HEIGHT;

	DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));

	/* generate keycode string */
	sprintf(keycodeString,"%d",keycode);

	/* draw the string */
	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/16 + 2*FRAME_THICKNESS, 2.5 * screenHeight/4+FONT_HEIGHT/2 + 4 * FRAME_THICKNESS, DSTF_LEFT));


	DFBCHECK(fontInterface->Release(fontInterface));
	// END REPEAT
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

GraphicsControllerError drawVolume(uint16_t volume) {
	int32_t ret;
	IDirectFBImageProvider *provider;
	IDirectFBSurface *logoSurface = NULL;
	int32_t logoHeight, logoWidth;
	char img_name[50];
	/* draw image from file */
	/* get image name */
	sprintf(img_name, "volume_%u.png", volume);
	printf("IMG NAME: %s\n", img_name);
	/* create the image provider for the specified file */
	DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, img_name, &provider));
	/* get surface descriptor for the surface where the image will be rendered */
	DFBCHECK(provider->GetSurfaceDescription(provider, &surfaceDesc));
	/* create the surface for the image */
	DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &logoSurface));
	/* render the image to the surface */
	DFBCHECK(provider->RenderTo(provider, logoSurface, NULL));


	/* cleanup the provider after rendering the image to the surface */
	provider->Release(provider);

	/* fetch the logo size and add (blit) it to the screen */
	DFBCHECK(logoSurface->GetSize(logoSurface, &logoWidth, &logoHeight));
	DFBCHECK(primary->Blit(primary,
						   /*source surface*/ logoSurface,
						   /*source region, NULL to blit the whole surface*/ NULL,
						   /*destination x coordinate of the upper left corner of the image*/50,
						   /*destination y coordinate of the upper left corner of the image*/50));


	/* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary,
						   /*region to be updated, NULL for the whole surface*/NULL,
						   /*flip flags*/0));
	// REPEAT
   /* get image name */
	sprintf(img_name, "volume_%u.png", volume);
	printf("IMG NAME: %s\n", img_name);
	/* create the image provider for the specified file */
	DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, img_name, &provider));
	/* get surface descriptor for the surface where the image will be rendered */
	DFBCHECK(provider->GetSurfaceDescription(provider, &surfaceDesc));
	/* create the surface for the image */
	DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &logoSurface));
	/* render the image to the surface */
	DFBCHECK(provider->RenderTo(provider, logoSurface, NULL));


	/* cleanup the provider after rendering the image to the surface */
	provider->Release(provider);

	/* fetch the logo size and add (blit) it to the screen */
	DFBCHECK(logoSurface->GetSize(logoSurface, &logoWidth, &logoHeight));
	DFBCHECK(primary->Blit(primary,
						   /*source surface*/ logoSurface,
						   /*source region, NULL to blit the whole surface*/ NULL,
						   /*destination x coordinate of the upper left corner of the image*/50,
						   /*destination y coordinate of the upper left corner of the image*/50));


	//END REPEAT

	/* set the timer for clearing the screen */
	//DFBCHECK(primary->SetColor(primary, 0x10, 0x80, 0x40, 0x00));
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