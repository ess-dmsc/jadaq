cmake_minimum_required(VERSION 3.5)
project(jadaq)

set(CMAKE_CXX_STANDARD 11)
set(SOURCE_FILES main.cpp caen.hpp)

add_library(debugCAENComm SHARED debugCAENComm.c)
target_link_libraries(debugCAENComm dl)

add_executable(jadaq ${SOURCE_FILES})
target_link_libraries(jadaq CAENComm CAENDigitizer CAENVME)
