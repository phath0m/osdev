AS=nasm
CC=i686-elysium-gcc
LD=i686-elysium-ld
AR=i686-elysium-ar
AFLAGS= -f elf32
CFLAGS = -c -std=gnu99 --freestanding -g -Wall -Werror -nostartfiles -nodefaultlibs -DELYSIUM_TARGET="\"i686\""
CFLAGS += -I "$(shell realpath ./i686/include)"

ifdef USE_BOOTLOADER_GRAPHICS
    AFLAGS += -DUSE_BOOTLOADER_GRAPHICS
    CFLAGS += -DUSE_BOOTLOADER_GRAPHICS
    CFLAGS += -DENABLE_DEV_LFB
else
    CFLAGS += -DENABLE_DEV_VGA
endif

CFLAGS += -DENABLE_DEV_KBD -DENABLE_DEV_MOUSE -DENABLE_DEV_SERIAL

export AS CC LD AR AFLAGS CFLAGS
