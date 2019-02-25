#include "remote_controller.h"

static int32_t inputFileDesc;
static void* inputEventTask();
static int32_t getKey(uint8_t* buf);
static pthread_t remote;
static uint8_t threadExit = 0;
static RemoteControllerCallback callback = NULL;

RemoteControllerError remoteControllerInit()
{

	/* handle input events in background process*/
	if (pthread_create(&remote, NULL, &inputEventTask, NULL))
	{
		printf("Error creating input event task!\n");
		return RC_THREAD_ERROR;
	}

	return RC_NO_ERROR;
}

RemoteControllerError remoteControllerDeinit()
{
	/* wait for EXIT key press input event*/
	threadExit = 1;
	if (pthread_join(remote, NULL))
	{
		printf("Error during thread join!\n");
		return RC_THREAD_ERROR;
	}
	return RC_NO_ERROR;
}

RemoteControllerError registerRemoteControllerCallback(RemoteControllerCallback remoteControllerCallback)
{
	callback = remoteControllerCallback;
	return RC_NO_ERROR;
}

RemoteControllerError unregisterRemoteControllerCallback(RemoteControllerCallback remoteControllerCallback)
{
	callback = NULL;
	return RC_NO_ERROR;
}

void* inputEventTask()
{
	char deviceName[20];
	struct input_event eventBuf;
	int32_t counter = 0;
	const char* dev = "/dev/input/event0";

	inputFileDesc = open(dev, O_RDWR);
	if(inputFileDesc == -1)
	{
		printf("Error while opening device (%s) !", strerror(errno));
		return (void*)RC_ERROR;
	}

	/* get the name of input device */
	ioctl(inputFileDesc, EVIOCGNAME(sizeof(deviceName)), deviceName);
	printf("RC device opened succesfully [%s]\n", deviceName);

	while(!threadExit)
	{

		/* read next input event */
		if(getKey((uint8_t*)&eventBuf))
		{
			printf("Error while reading input events !");
			return (void*)RC_ERROR;
		}


		/* filter input events */
		if(eventBuf.type == EV_KEY &&
		  (eventBuf.value == EV_VALUE_KEYPRESS || eventBuf.value == EV_VALUE_AUTOREPEAT))
		{
			printf("Event time: %d sec, %d usec\n",(int)eventBuf.time.tv_sec,(int)eventBuf.time.tv_usec);
			printf("Event type: %hu\n",eventBuf.type);
			printf("Event code: %hu\n",eventBuf.code);
			printf("Event value: %d\n",eventBuf.value);
			printf("\n");

			/* trigger remote controller callback */
			if (callback) {
				callback(eventBuf.code, eventBuf.type, eventBuf.value);
			}

		}
	}
	return (void*)RC_NO_ERROR;
}

int32_t getKey(uint8_t* buf)
{
	int32_t ret = 0;

	/* read next input event and put it in buffer */
	ret = read(inputFileDesc, buf, (size_t)(sizeof(struct input_event)));
	if(ret <= 0)
	{
		printf("Error code %d", ret);
		return RC_ERROR;
	}

	return RC_NO_ERROR;
}
