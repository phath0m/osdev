AUTOMAKE_OPTIONS = cygnus
INCLUDES = $(NEWLIB_CFLAGS) $(CROSS_CFLAGS) $(TARGET_CFLAGS)
AM_CCASFLAGS = $(INCLUDES)
 
noinst_LIBRARIES = lib.a
 
if MAY_SUPPLY_SYSCALLS
extra_objs = dirent.o group.o libgen.o passwd.o premain.o getline.o signal.o shadow.o syscalls.o termios.o crt0.o # add more object files here if you split up
else                    # syscalls.c into multiple files in the previous step
extra_objs =
endif
 
lib_a_SOURCES =
lib_a_LIBADD = $(extra_objs)
EXTRA_lib_a_SOURCES = dirent.c group.c libgen.c getline.o shadow.c signal.c syscalls.c passwd.c premain.c termios.c # add more source files here if you split up
lib_a_DEPENDENCIES = $(extra_objs)      # syscalls.c into multiple files
lib_a_CCASFLAGS = $(AM_CCASFLAGS)
lib_a_CFLAGS = $(AM_CFLAGS)
 
if MAY_SUPPLY_SYSCALLS
all:
endif
 
ACLOCAL_AMFLAGS = -I ../../..
CONFIG_STATUS_DEPENDENCIES = $(newlib_basedir)/configure.host
