#include "volume_controller.h"

static uint32_t player = -1;
static uint16_t volume_level = 0;

VolumeControllerError volumeControllerInit(uint32_t playerHandle) {
	player = playerHandle;
	if (Player_Volume_Set(player, INIT_VOLUME * VOLUME_LEVEL)) {
		return VC_ERROR;
	}

	volume_level = INIT_VOLUME;

	return VC_NO_ERROR;
}

VolumeControllerError volumeUp() {
	if (volume_level < 10) {
		volume_level++;
	} else {
		volume_level = 10;
	}
	if (Player_Volume_Set(player, volume_level * VOLUME_LEVEL)) {
		return VC_ERROR;
	}

	printf("New volume_level: %hu", volume_level);

	return VC_NO_ERROR;
}

VolumeControllerError volumeDown() {
	if (volume_level > 0) {
		volume_level--;
	} else {
		volume_level = 0;
	}
	if (Player_Volume_Set(player, volume_level * VOLUME_LEVEL)) {
		return VC_ERROR;
	}

	printf("New volume_level: %hu", volume_level);

	return VC_NO_ERROR;
}

VolumeControllerError getVolume(uint32_t *vol) {
	uint32_t volume;

	if (Player_Volume_Get(player, &volume)) {
		return VC_ERROR;
	}

	*vol = volume;

	return VC_NO_ERROR;
}

VolumeControllerError muteVolume() {
	volume_level = 0;
	if (Player_Volume_Set(player, 0)) {
		return VC_ERROR;
	}

	return VC_NO_ERROR;
}