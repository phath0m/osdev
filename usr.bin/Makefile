export CC=i686-elysium-gcc
export LD=i686-elysium-gcc

MAKE = make

SUBDIRS += kstat
SUBDIRS += unlink
SUBDIRS += grep
SUBDIRS += login

all: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir);)
clean: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) clean;)
