cmake_minimum_required(VERSION 3.2)
project(jadaq)
#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

find_path(CAEN_INCLUDE_DIR NAMES CAENDigitizer.h
        HINTS   $ENV{HOME}/include)
find_library(CAEN_LIB NAMES CAENDigitizer
        HINTS   $ENV{HOME}/lib)
include_directories(${CAEN_INCLUDE_DIR})

add_library(debugCAENComm SHARED debugCAENComm.c)
target_link_libraries(debugCAENComm dl)

add_library(caen SHARED caen.hpp caen.cpp _CAENDigitizer.c _CAENDigitizer.h)
target_link_libraries(caen dl)

add_executable(jadaq jadaq.cpp caen.hpp Configuration.cpp Configuration.hpp Digitizer.cpp Digitizer.hpp FunctionID.hpp FunctionID.cpp ini_parser.hpp)
target_link_libraries(jadaq ${CAEN_LIB} caen)
