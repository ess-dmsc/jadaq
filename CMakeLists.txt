cmake_minimum_required(VERSION 3.2)
project(jadaq)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES jadaq.cpp caen.hpp caen.cpp)

add_library(debugCAENComm SHARED debugCAENComm.c)
target_link_libraries(debugCAENComm dl)

add_library(caen SHARED caen.hpp caen.cpp)
target_link_libraries(caen dl)

add_executable(jadaq ${SOURCE_FILES})
target_link_libraries(jadaq CAENComm CAENDigitizer CAENVME caen)
