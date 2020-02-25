AS=nasm
CC=i686-elysium-gcc
LD=i686-elysium-ld
AR=i686-elysium-ar
AFLAGS= -f elf32
CFLAGS = -c -std=gnu99 --freestanding -g -Wall -Werror -nostartfiles -nodefaultlibs -DELYSIUM_TARGET="\"i686\""
CFLAGS += -I "$(shell realpath ./i686/include)"

export AS CC LD AR AFLAGS CFLAGS
