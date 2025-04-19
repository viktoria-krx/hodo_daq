cmake_minimum_required(VERSION 3.5.0)
project(hodo_analysis)


include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc ${PROJECT_SOURCE_DIR}/src/*.c)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh ${PROJECT_SOURCE_DIR}/include/*.h)

set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#set(CAENVMELIB_PATH "/home/viktoria/software/CAEN/CAENVMELib-v4.0.2/")
#set(CAENPLULIB_PATH "/home/viktoria/software/CAEN/CAEN_PLU-1.2")

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} /home/hododaq/software/spdlog/build/)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} /home/hododaq/software/zmq/cppzmq/build/)
#find_library(CAENVMELIB NAMES CAENVMELib PATHS /usr/lib/libCAENVME.so)
find_package(spdlog REQUIRED)
find_package(cppzmq REQUIRED)
find_package(ROOT)

#if (NOT CAENVMELIB)
#    message(FATAL_ERROR "CAENVME library not found in ${CAENVMELIB_PATH}. Check the path!")
#endif()


#include_directories(${CAENVMELIB_PATH}/include)
#include_directories(${CAENPLULIB_PATH}/include)

add_executable(hodo_analysis analysis.cc ${sources})
target_link_libraries(hodo_analysis spdlog::spdlog)
target_link_libraries(hodo_analysis cppzmq)
target_link_libraries(hodo_analysis ${ROOT_LIBRARIES})
