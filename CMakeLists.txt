cmake_minimum_required(VERSION 3.5.0)
project(hodo_daq)


include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc ${PROJECT_SOURCE_DIR}/src/*.c)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh ${PROJECT_SOURCE_DIR}/include/*.h)

set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

set(CAENVMELIB_PATH "/home/viktoria/software/CAEN/CAENVMELib-v3.4.3/")
set(CAENPLULIB_PATH "/home/viktoria/software/CAEN/CAEN_PLU-1.2")

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} /home/viktoria/Documents/Hodoscope/blt_frontend/spdlog/build/)

#find_library(CAENVMELIB NAMES CAENVMELib PATHS /usr/lib/libCAENVME.so)
find_package(spdlog REQUIRED)

#if (NOT CAENVMELIB)
#    message(FATAL_ERROR "CAENVME library not found in ${CAENVMELIB_PATH}. Check the path!")
#endif()


include_directories(${CAENVMELIB_PATH}/include)
include_directories(${CAENPLULIB_PATH}/include)

add_executable(hodo_daq daq_controller.cc ${sources})
target_link_libraries(hodo_daq /usr/lib/libCAENVME.so /usr/lib/libCAEN_PLU.so spdlog::spdlog)