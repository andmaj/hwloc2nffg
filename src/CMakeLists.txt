cmake_minimum_required (VERSION 2.6)
project(hwloc2nffg_bin)

find_package(Boost REQUIRED program_options filesystem regex)
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "-std=c++11 -DBOOST_SYSTEM_NO_DEPRECATED")

add_executable(hwloc2nffg hwloc2nffg.cpp dpdk-query.cpp interface-query.cpp)
target_link_libraries(hwloc2nffg ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(hwloc2nffg ${Boost_FILESYSTEM_LIBRARY})
target_link_libraries(hwloc2nffg ${Boost_REGEX_LIBRARY})
target_link_libraries(hwloc2nffg "jsoncpp")
target_link_libraries(hwloc2nffg "hwloc")
