cmake_minimum_required(VERSION 3.2)
project(jadaq)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

add_library(debugCAENComm SHARED debugCAENComm.c)
target_link_libraries(debugCAENComm dl)

add_library(caen SHARED caen.hpp caen.cpp _CAENDigitizer.c _CAENDigitizer.h)
target_link_libraries(caen dl)

add_executable(jadaq jadaq.cpp caen.hpp Configuration.cpp Configuration.hpp Settings.hpp Settings.cpp)
target_link_libraries(jadaq CAENComm CAENDigitizer CAENVME caen)
