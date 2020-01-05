export CC=i686-elysium-gcc
export LD=i686-elysium-gcc

MAKE = make

SUBDIRS += cat
SUBDIRS += echo
SUBDIRS += ls
SUBDIRS += sh
SUBDIRS += ps
SUBDIRS += mkdir
SUBDIRS += rmdir

all: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir);)
clean: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) clean;)
