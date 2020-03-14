export CC=i686-elysium-gcc
export LD=i686-elysium-gcc
export DESTDIR
export PREFIX

include config.mk

MAKE = make

all: $(PORTS)
	$(foreach subdir,$(PORTS),$(MAKE) -C $(subdir);)
install: $(PORTS)
	mkdir -p "$(DESTDIR)/$(PREFIX)"
	$(foreach subdir,$(PORTS),$(MAKE) -C $(subdir) install;)
clean: $(PORTS)
	$(foreach subdir,$(PORTS),$(MAKE) -C $(subdir) clean;)
