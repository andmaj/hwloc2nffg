# hwloc2nffg

## Description
Converts hwloc data (hierarchical map of computing and network elements)
to an NFFG representation in JSON format. This data can be processed by
an orchestrator. 

## Requirements
* C++11 compiler (G++ preferred)
* cmake
* make
* boost-program-options library
* hwloc library
* jsoncpp library

```
On Debian Jessie you can install them by:
apt-get install g++ cmake make libboost-program-options-dev libhwloc-dev libjsoncpp-dev
```

## Build
```
cd build
cmake ../
make
```

## Run (in build directory)
```
./bin/hwloc2nffg > machine.nffg
```

## Author
```
Written by Andras Majdan.
Email: majdan.andras@gmail.com
```

