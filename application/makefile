CROSS_COMPILE=$(TOOLCHAIN_CROSS_COMPILE)

CC_PREFIX=$(CROSS_COMPILE)-
CC=$(CC_PREFIX)gcc
CXX=$(CC_PREFIX)g++
LD=$(CC_PREFIX)ld
ROOTFS_PATH=$(SDK_ROOTFS)

SYSROOT=$(SDK_ROOTFS)
GALOIS_INCLUDE=$(SDK_GALOIS)

INCS =	-I./../../tdp_api
INCS += -I./include/ 							\
		-I$(SYSROOT)/usr/include/         \
		-I$(GALOIS_INCLUDE)/Common/include/     \
		-I$(GALOIS_INCLUDE)/OSAL/include/		\
		-I$(GALOIS_INCLUDE)/OSAL/include/CPU1/	\
		-I$(GALOIS_INCLUDE)/PE/Common/include/  \
		-I$(ROOTFS_PATH)/usr/include/directfb/

LIBS_PATH = -L./../../tdp_api

LIBS_PATH += -L$(SYSROOT)/home/galois/lib/
LIBS_PATH += -L$(ROOTFS_PATH)/home/galois/lib/directfb-1.4-6-libs

LIBS := $(LIBS_PATH) -ltdp -ldirectfb -ldirect -lfusion -lrt

LIBS += $(LIBS_PATH) -lOSAL	-lshm -lPEAgent

CFLAGS += -D__LINUX__ -O0 -Wno-psabi --sysroot=$(SYSROOT)

CXXFLAGS = $(CFLAGS)

all: parser_playback_sample

SRCS =  ./main.c
SRCS += ./tables_parser.c ./remote_controller.c ./stream_controller.c ./graphics_controller.c ./init_controller.c ./volume_controller.c

parser_playback_sample:
	$(CC) -o main $(INCS) $(SRCS) $(CFLAGS) $(LIBS)
	cp main ../../../ploca
	cp assets/* ../../../ploca
clean:
	rm -f main
