# Requires a C99 compiler (GCC/CLANG preferred),
# a make system (GNU Make preferred) and pkgconfig.
#
# Hwloc and libjson-c developer packages must be installed.
# On Debian Jessie they are called libhwloc-dev and libjson-c-dev.

CFLAGS += $(shell pkg-config --cflags hwloc)
CFLAGS += $(shell pkg-config --cflags json-c)
LDFLAGS += $(shell pkg-config --libs hwloc)
LDFLAGS += $(shell pkg-config --libs json-c)

all:
	cc -std=c99 $(CFLAGS) $(LDFLAGS) -o bin/hwloc2nffg src/hwloc2nffg.c

clean:
	rm -f bin/hwloc2nffg
