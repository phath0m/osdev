export CC=i686-elysium-gcc
export LD=i686-elysium-gcc
export CFLAGS=-c -Wall -Werror -Wdeclaration-after-statement
export LDFLAGS
export PREFIX
export DESTDIR

MAKE = make

SUBDIRS += cat
SUBDIRS += chmod
SUBDIRS += chown
SUBDIRS += cp
SUBDIRS += date
SUBDIRS += dmesg
SUBDIRS += echo
SUBDIRS += expr
SUBDIRS += kill
SUBDIRS += ls
SUBDIRS += sh
SUBDIRS += sha1
SUBDIRS += test
SUBDIRS += ps
SUBDIRS += pwd
SUBDIRS += mkdir
SUBDIRS += rm
SUBDIRS += rmdir

all: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir);)
install: $(SUBDIRS)
	mkdir -p "$(DESTDIR)/$(PREFIX)/bin"
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) install;)
clean: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) clean;)
