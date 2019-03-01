#include "stream_controller.h"

static PatTable *pat_table;
static PmtTable *pmt_table;
static EitTable *eit_table;
static pthread_cond_t status_condition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;

static int32_t sectionReceivedCallback(uint8_t *buffer);
static int32_t tunerStatusCallback(t_LockStatus status);

static uint32_t player_handle = 0;
static uint32_t source_handle = 0;
static uint32_t stream_handle_a = 0;
static uint32_t stream_handle_v = 0;
static uint32_t filter_handle = 0;
static uint8_t thread_exit = 0;
static bool change_channel = false;
static int16_t program_number = 0;
static ChannelInfo current_channel;
static bool is_initialized = false;

static struct timespec lock_status_wait_time;
static struct timeval now;
static pthread_t sc_thread;
static pthread_cond_t demux_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t demux_mutex = PTHREAD_MUTEX_INITIALIZER;

static void* streamControllerTask();
static void startChannel(int32_t channel_number);

static uint32_t sc_freq;
static uint32_t sc_bandwidth;
static t_Module sc_module;
static ChannelT sc_channel;
static uint16_t sc_program_no;

static char current_name[100] = "";

static char next_name[100] = "";

static update;

StreamControllerError streamControllerInit(uint32_t freq, uint32_t bandwidth,  t_Module module, ChannelT channel, uint16_t program_no)
{
	if (pthread_create(&sc_thread, NULL, &streamControllerTask, NULL))
	{
		printf("Error creating input event task!\n");
		return SC_THREAD_ERROR;
	}

	sc_freq = freq;
	sc_bandwidth = bandwidth;
	sc_module = module;
	sc_channel = channel;
	sc_program_no = program_no;

	return SC_NO_ERROR;
}

StreamControllerError streamControllerDeinit()
{
	if (!is_initialized)
	{
		printf("\n%s : ERROR streamControllerDeinit() fail, module is not initialized!\n", __FUNCTION__);
		return SC_ERROR;
	}

	thread_exit = 1;
	if (pthread_join(sc_thread, NULL))
	{
		printf("\n%s : ERROR pthread_join fail!\n", __FUNCTION__);
		return SC_THREAD_ERROR;
	}

	/* free demux filter */
	Demux_Free_Filter(player_handle, filter_handle);

	/* remove audio stream */
	Player_Stream_Remove(player_handle, source_handle, stream_handle_a);

	/* remove video stream */
	Player_Stream_Remove(player_handle, source_handle, stream_handle_v);

	/* close player source */
	Player_Source_Close(player_handle, source_handle);

	/* deinitialize player */
	Player_Deinit(player_handle);

	/* deinitialize tuner device */
	Tuner_Deinit();

	/* free allocated memory */
	free(pat_table);
	free(pmt_table);
    free(eit_table);

	/* set is_initialized flag */
	is_initialized = false;

	return SC_NO_ERROR;
}

StreamControllerError channelUp()
{
	if (program_number >= pat_table->serviceInfoCount - 2)
	{
		program_number = 0;
	}
	else
	{
		program_number++;
	}

	/* set flag to start current channel */
	change_channel = true;

	return SC_NO_ERROR;
}

StreamControllerError SetChannel(int32_t channel_number)
{
	if (channel_number  < pat_table->serviceInfoCount - 1 && channel_number > 1) {
		program_number = channel_number - 1;
		/* set flag to start current channel */
		change_channel = true;
	}

	return SC_NO_ERROR;
}

StreamControllerError channelDown()
{
	if (program_number <= 0)
	{
		program_number = pat_table->serviceInfoCount - 2;
	}
	else
	{
		program_number--;
	}

	/* set flag to start current channel */
	change_channel = true;

	return SC_NO_ERROR;
}

StreamControllerError getChannelInfo(ChannelInfo* channelInfo)
{
	if (channelInfo == NULL)
	{
		printf("\n Error wrong parameter\n", __FUNCTION__);
		return SC_ERROR;
	}

	channelInfo->programNumber = current_channel.programNumber;
	channelInfo->audioPid = current_channel.audioPid;
	channelInfo->videoPid = current_channel.videoPid;
	channelInfo->isRadio = current_channel.isRadio;
	channelInfo->teletext = current_channel.teletext;
	strcpy(channelInfo->currentInfo, current_name);
	strcpy(channelInfo->nextInfo, next_name);

	return SC_NO_ERROR;
}

/* Sets filter to receive current channel PMT table
 * Parses current channel PMT table when it arrives
 * Creates streams with current channel audio and video pids
 */
void startChannel(int32_t channel_number)
{
	strcpy(current_name, "");
	strcpy(next_name, "");
	update = true;

	/* free PAT table filter */
	Demux_Free_Filter(player_handle, filter_handle);

	/* set demux filter for receive PMT table of program */
	if(Demux_Set_Filter(player_handle, pat_table->patServiceInfoArray[channel_number + 1].pid, 0x02, &filter_handle))
	{
		printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
		return;
	}

	/* wait for a PMT table to be parsed*/
	pthread_mutex_lock(&demux_mutex);
	if (ETIMEDOUT == pthread_cond_wait(&demux_cond, &demux_mutex))
	{
		printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
		streamControllerDeinit();
	}
	pthread_mutex_unlock(&demux_mutex);

	/* get audio and video pids */
	int16_t audio_pid = -1;
	int16_t video_pid = -1;
	bool teletext = false;
	uint8_t i = 0;
	for (i = 0; i < pmt_table->elementaryInfoCount; i++)
	{
		if (((pmt_table->pmtElementaryInfoArray[i].streamType == 0x1) || (pmt_table->pmtElementaryInfoArray[i].streamType == 0x2) || (pmt_table->pmtElementaryInfoArray[i].streamType == 0x1b))
			&& (video_pid == -1))
		{
			video_pid = pmt_table->pmtElementaryInfoArray[i].elementaryPid;
		}
		else if (((pmt_table->pmtElementaryInfoArray[i].streamType == 0x3) || (pmt_table->pmtElementaryInfoArray[i].streamType == 0x4))
			&& (audio_pid == -1))
		{
			audio_pid = pmt_table->pmtElementaryInfoArray[i].elementaryPid;
		}
        if(pmt_table->pmtElementaryInfoArray[i].streamType == 0x6)
        {
        	teletext = true;
        }
	}

	if (video_pid != -1)
	{
		/* remove previous video stream */
		if (stream_handle_v != 0)
		{
			Player_Stream_Remove(player_handle, source_handle, stream_handle_v);
			stream_handle_v = 0;
		}

		/* create video stream */
		if(Player_Stream_Create(player_handle, source_handle, video_pid, VIDEO_TYPE_MPEG2, &stream_handle_v))
		{
			printf("\n%s : ERROR Cannot create video stream\n", __FUNCTION__);
			streamControllerDeinit();
		}
	} else {
		/* remove previous video stream */
		if (stream_handle_v != 0)
		{
			Player_Stream_Remove(player_handle, source_handle, stream_handle_v);
			stream_handle_v = 0;
		}
	}

	if (audio_pid != -1)
	{
		/* remove previos audio stream */
		if (stream_handle_a != 0)
		{
			Player_Stream_Remove(player_handle, source_handle, stream_handle_a);
			stream_handle_a = 0;
		}

		/* create audio stream */
		if(Player_Stream_Create(player_handle, source_handle, audio_pid, AUDIO_TYPE_MPEG_AUDIO, &stream_handle_a))
		{
			printf("\n%s : ERROR Cannot create audio stream\n", __FUNCTION__);
			streamControllerDeinit();
		}
	}

	/* store current channel info */
	current_channel.programNumber = channel_number + 1;
	current_channel.audioPid = audio_pid;
	current_channel.videoPid = video_pid;
	current_channel.teletext = teletext;


	/* free EIT table filter*/
	Demux_Free_Filter(player_handle, filter_handle);

	/*set demux filter for receive EIT table */

	if(Demux_Set_Filter(player_handle, 0x12, 0x4E, &filter_handle))
	{
		printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
		return;
	}

	pthread_mutex_lock(&demux_mutex);
	pthread_cond_signal(&demux_cond);
	pthread_mutex_unlock(&demux_mutex);

	if (video_pid != -1) {
		current_channel.isRadio = false;
		drawChannelInfo(false, channel_number + 1, audio_pid, video_pid, teletext, current_name, next_name);
	} else {
		current_channel.isRadio = true;
		drawChannelInfo(true, channel_number + 1, audio_pid, video_pid, teletext, current_name, next_name);
	}
}

void* streamControllerTask()
{
	gettimeofday(&now,NULL);
	lock_status_wait_time.tv_sec = now.tv_sec+10;

	/* allocate memory for PAT table section */
	pat_table=(PatTable*)malloc(sizeof(PatTable));
	if(pat_table==NULL)
	{
		printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
		return (void*) SC_ERROR;
	}
	memset(pat_table, 0x0, sizeof(PatTable));

	/* allocate memory for PMT table section */
	pmt_table=(PmtTable*)malloc(sizeof(PmtTable));
	if(pmt_table==NULL)
	{
		printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
		return (void*) SC_ERROR;
	}
	memset(pmt_table, 0x0, sizeof(PmtTable));

	/* allocate memory for PMT table section */
	eit_table=(EitTable*)malloc(sizeof(EitTable));
	if(eit_table==NULL)
	{
		printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
		return (void*) SC_ERROR;
	}
	memset(eit_table, 0x0, sizeof(EitTable));

	/* initialize tuner device */
	if(Tuner_Init())
	{
		printf("\n%s : ERROR Tuner_Init() fail\n", __FUNCTION__);
		free(pat_table);
		free(pmt_table);
		free(eit_table);
		return (void*) SC_ERROR;
	}

	/* register tuner status callback */
	if(Tuner_Register_Status_Callback(tunerStatusCallback))
	{
		printf("\n%s : ERROR Tuner_Register_Status_Callback() fail\n", __FUNCTION__);
	}

	/* lock to frequency */
	if(!Tuner_Lock_To_Frequency(sc_freq, sc_bandwidth, sc_module))
	{
		printf("\n%s: INFO Tuner_Lock_To_Frequency(): %d Hz - success!\n",__FUNCTION__,sc_freq);
	}
	else
	{
		printf("\n%s: ERROR Tuner_Lock_To_Frequency(): %d Hz - fail!\n",__FUNCTION__,sc_freq);
		free(pat_table);
		free(pmt_table);
		free(eit_table);
		Tuner_Deinit();
		return (void*) SC_ERROR;
	}

	/* wait for tuner to lock */
	pthread_mutex_lock(&status_mutex);
	if(ETIMEDOUT == pthread_cond_timedwait(&status_condition, &status_mutex, &lock_status_wait_time))
	{
		printf("\n%s : ERROR Lock timeout exceeded!\n",__FUNCTION__);
		free(pat_table);
		free(pmt_table);
		free(eit_table);
		Tuner_Deinit();
		return (void*) SC_ERROR;
	}
	pthread_mutex_unlock(&status_mutex);

	/* initialize player */
	if(Player_Init(&player_handle))
	{
		printf("\n%s : ERROR Player_Init() fail\n", __FUNCTION__);
		free(pat_table);
		free(pmt_table);
		free(eit_table);
		Tuner_Deinit();
		return (void*) SC_ERROR;
	}

	/* initialize volume controller */
	if (volumeControllerInit(player_handle)) {
		printf("\n%s : ERROR volumeControllerInit() fail\n", __FUNCTION__);
		free(pat_table);
		free(pmt_table);
		free(eit_table);
		Player_Deinit(player_handle);
		Tuner_Deinit();
		return (void*) SC_ERROR;
	}

	/* open source */
	if(Player_Source_Open(player_handle, &source_handle))
	{
		printf("\n%s : ERROR Player_Source_Open() fail\n", __FUNCTION__);
		free(pat_table);
		free(pmt_table);
		free(eit_table);
		Player_Deinit(player_handle);
		Tuner_Deinit();
		return (void*) SC_ERROR;
	}

	/* set PAT pid and tableID to demultiplexer */
	if(Demux_Set_Filter(player_handle, 0x00, 0x00, &filter_handle))
	{
		printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
	}

	/* register section filter callback */
	if(Demux_Register_Section_Filter_Callback(sectionReceivedCallback))
	{
		printf("\n%s : ERROR Demux_Register_Section_Filter_Callback() fail\n", __FUNCTION__);
	}

	pthread_mutex_lock(&demux_mutex);
	if (ETIMEDOUT == pthread_cond_wait(&demux_cond, &demux_mutex))
	{
		printf("\n%s:ERROR Lock timeout exceeded!\n", __FUNCTION__);
		free(pat_table);
		free(pmt_table);
		free(eit_table);
		Player_Deinit(player_handle);
		Tuner_Deinit();
		return (void*) SC_ERROR;
	}
	pthread_mutex_unlock(&demux_mutex);

	/* start current channel */
	startChannel(program_number);

	/* set is_initialized flag */
	is_initialized = true;

	while(!thread_exit)
	{
		if (change_channel)
		{
			change_channel = false;
			startChannel(program_number);
		}
	}
}

int32_t sectionReceivedCallback(uint8_t *buffer)
{
	uint8_t tableId = *buffer;
	if(tableId==0x00)
	{
		if(parsePatTable(buffer,pat_table)==TABLES_PARSE_OK)
		{
			pthread_mutex_lock(&demux_mutex);
			pthread_cond_signal(&demux_cond);
			pthread_mutex_unlock(&demux_mutex);
		}
	}
	else if (tableId==0x02)
	{
		if(parsePmtTable(buffer,pmt_table) == TABLES_PARSE_OK)
		{
			pthread_mutex_lock(&demux_mutex);
			pthread_cond_signal(&demux_cond);
			pthread_mutex_unlock(&demux_mutex);
		}
	}
	else if (tableId == 0x4E)
	{
		if(parseEitTable(buffer,eit_table) == TABLES_PARSE_OK)
		{
			int i;
			if (eit_table->eitHeader.serviceId == pmt_table->pmtHeader.programNumber) {
				for (i = 0; i < eit_table->eventsInfoCount; i++) {
					if (eit_table->eitInfoArray[i].runningStatus == 0x04)
					{
						strcpy(current_name, eit_table->eitInfoArray[i].descriptor.eventNameChar);
					}
					else if (eit_table->eitInfoArray[i].runningStatus == 0x01)
					{
						strcpy(next_name, eit_table->eitInfoArray[i].descriptor.eventNameChar);
					}
				}
			}
			strcpy(current_channel.currentInfo, current_name);
			strcpy(current_channel.nextInfo, next_name);
			if (update && strcmp(next_name, "") && strcmp(current_name, "")) {
				updateChannelInfo(current_channel.programNumber, current_channel.audioPid, current_channel.videoPid, current_channel.teletext, current_channel.currentInfo, current_channel.nextInfo);
				update = false;
			}
		}
	}
	return 0;
}

int32_t tunerStatusCallback(t_LockStatus status)
{
	if(status == STATUS_LOCKED)
	{
		pthread_mutex_lock(&status_mutex);
		pthread_cond_signal(&status_condition);
		pthread_mutex_unlock(&status_mutex);
		printf("\n%s -----TUNER LOCKED-----\n",__FUNCTION__);
	}
	else
	{
		printf("\n%s -----TUNER NOT LOCKED-----\n",__FUNCTION__);
	}
	return 0;
}
