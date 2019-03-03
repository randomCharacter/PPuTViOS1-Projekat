#include "remote_controller.h"

static int32_t input_file_desc;
static void *inputEventTask();
static int32_t getKey(uint8_t *buf);
static pthread_t remote;
static uint8_t thread_exit = 0;
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
	thread_exit = 1;
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

void *inputEventTask()
{
	char device_name[20];
	struct input_event event_buf;
	int32_t counter = 0;
	const char *dev = "/dev/input/event0";

	input_file_desc = open(dev, O_RDWR);
	if (input_file_desc == -1)
	{
		printf("Error while opening device (%s) !", strerror(errno));
		return (void *)RC_ERROR;
	}

	/* get the name of input device */
	ioctl(input_file_desc, EVIOCGNAME(sizeof(device_name)), device_name);
	printf("RC device opened succesfully [%s]\n", device_name);

	while (!thread_exit)
	{

		/* read next input event */
		if (getKey((uint8_t *)&event_buf))
		{
			printf("Error while reading input events !");
			return (void *)RC_ERROR;
		}

		/* filter input events */
		if (event_buf.type == EV_KEY &&
			(event_buf.value == EV_VALUE_KEYPRESS || event_buf.value == EV_VALUE_AUTOREPEAT))
		{
			printf("Event time: %d sec, %d usec\n", (int)event_buf.time.tv_sec, (int)event_buf.time.tv_usec);
			printf("Event type: %hu\n", event_buf.type);
			printf("Event code: %hu\n", event_buf.code);
			printf("Event value: %d\n", event_buf.value);
			printf("\n");

			/* trigger remote controller callback */
			if (callback)
			{
				callback(event_buf.code, event_buf.type, event_buf.value);
			}
		}
	}
	return (void *)RC_NO_ERROR;
}

int32_t getKey(uint8_t *buf)
{
	int32_t ret = 0;

	/* read next input event and put it in buffer */
	ret = read(input_file_desc, buf, (size_t)(sizeof(struct input_event)));
	if (ret <= 0)
	{
		printf("Error code %d", ret);
		return RC_ERROR;
	}

	return RC_NO_ERROR;
}
