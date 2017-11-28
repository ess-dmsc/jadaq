cmake_minimum_required(VERSION 3.2)
project(jadaq)
#set(CMAKE_VERBOSE_MAKEFILE ON)
#
# NOTE: to compile for valgrind debug run something like:
# mkdir -p build
# cd build
# cmake -D CMAKE_BUILD_TYPE=Debug ..
# make
# valgrind --leak-check=full --track-origins=yes ./jadaq [CONFFILE]
#
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)
if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.3)
    message(FATAL_ERROR "Require at least g++-5.3: On CentOS-7 use Devtoolset-4")
endif()
find_path(CAEN_INCLUDE_DIR NAMES CAENDigitizer.h
        HINTS   $ENV{HOME}/include)
find_library(CAEN_LIB NAMES CAENDigitizer
        HINTS   $ENV{HOME}/lib)
include_directories(${CAEN_INCLUDE_DIR})

find_package(Boost COMPONENTS system filesystem REQUIRED)

add_library(debugCAENComm SHARED debugCAENComm.c)
target_link_libraries(debugCAENComm dl)

add_library(caen SHARED caen.hpp caen.cpp _CAENDigitizer.c _CAENDigitizer.h)
target_link_libraries(caen dl)

add_executable(jadaq jadaq.cpp caen.hpp Configuration.cpp Configuration.hpp Digitizer.cpp Digitizer.hpp FunctionID.hpp FunctionID.cpp ini_parser.hpp StringConversion.cpp StringConversion.hpp CAENConf.cpp CAENConf.hpp)
target_link_libraries(jadaq ${CAEN_LIB} caen ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})

# For hdf5writer
find_package(HDF5 REQUIRED COMPONENTS C CXX)
include_directories(${HDF5_INCLUDE_DIRS})
add_executable(hdf5writer hdf5writer.cpp)
target_link_libraries(hdf5writer ${HDF5_CXX_LIBRARIES})
