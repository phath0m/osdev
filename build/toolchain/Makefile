TOOLROOT = $(shell realpath ../toolroot)
AUTOCONF = $(TOOLROOT)/bin/autoconf
AUTOMAKE = $(TOOLROOT)/bin/automake
BINUTILS = $(TOOLROOT)/bin/i686-elysium-as
GCC = $(TOOLROOT)/bin/i686-elysium-gcc

export PREFIX=$(TOOLROOT)
export TOOLROOT

all: $(AUTOCONF) $(AUTOMAKE) $(BINUTILS) $(GCC)

$(AUTOCONF):
	PATH="$(TOOLROOT)/bin:$$PATH" make -C autoconf && make -C autoconf install
$(AUTOMAKE): $(AUTOCONF)
	PATH="$(TOOLROOT)/bin:$$PATH" make -C automake && make -C automake install
$(BINUTILS): $(AUTOMAKE)
	PATH="$(TOOLROOT)/bin:$$PATH" make -C binutils && make -C binutils install
$(GCC): $(BINUTILS)
	PATH="$(TOOLROOT)/bin:$$PATH" make -C gcc && make -C gcc install
