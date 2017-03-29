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
* boost-filesystem library
* boost-regex library
* hwloc library
* jsoncpp library

```
On Debian Jessie you can install them by:
apt-get install g++ cmake make libboost-program-options-dev libboost-filesystem-dev libboost-regex-dev libhwloc-dev libjsoncpp-dev
```

## Build
```
cd build
cmake ../
make
```

## Run (in build directory)
* Full graph 
```
./bin/hwloc2nffg > machine.nffg
```
* Merge node in case of one child
```
./bin/hwloc2nffg --merge > machine.nffg
```
* Include DPDK interfaces
```
./bin/hwloc2nffg --dpdk > machine.nffg
```
* Include not reported network interfaces
```
./bin/hwloc2nffg --notreported > machine.nffg
```
## Author
```
Written by Andras Majdan.
Email: majdan.andras@gmail.com
```

