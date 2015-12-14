# hwloc2nffg
Converts hwloc data (hierarchical map of computing and network elements) to an NFFG representation in JSON format. This data can be processed by an orchestrator. 

Requires a C99 compiler (GCC/CLANG preferred), a make system (GNU Make preferred) and pkgconfig. Hwloc and libjson-c developer packages must be installed. On Debian Jessie they are called libhwloc-dev and libjson-c-dev.

# Compile:
make

# Run:
./bin/hwloc2nffg > mynffg.json

Written by Andras Majdan.
Email: majdan.andras@gmail.com