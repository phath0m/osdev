MAKE = make

SUBDIRS += cat
SUBDIRS += echo
SUBDIRS += ls
SUBDIRS += sh

all: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir);)
clean: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) clean;)
