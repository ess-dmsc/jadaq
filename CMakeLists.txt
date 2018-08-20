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
set(ACCEPED_WARNINGS "-Wno-unused-function -Wno-unused-variable")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall ${ACCEPED_WARNINGS} -Werror" )
set(CMAKE_C_STANDARD 99)
if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.3)
    message(FATAL_ERROR "Require at least g++-5.3: On CentOS-7 use Devtoolset-4")
    # for further details please refer to
    # https://www.softwarecollections.org/en/scls/rhscl/devtoolset-4/
    # add something like the following in your shell rc to always use it:
    # source scl_source enable devtoolset-4
endif()
find_path(CAEN_INCLUDE_DIR NAMES CAENDigitizer.h
        HINTS   $ENV{HOME}/include)
find_library(CAEN_LIB NAMES CAENDigitizer
        HINTS   $ENV{HOME}/lib)
include_directories(${CAEN_INCLUDE_DIR})

find_package(Boost COMPONENTS system filesystem thread program_options REQUIRED)

find_package(HDF5 1.10 REQUIRED COMPONENTS C CXX HL)
include_directories(${HDF5_INCLUDE_DIRS})

add_library(debugCAENComm SHARED debugCAENComm.c)
target_link_libraries(debugCAENComm dl)

add_library(caen SHARED caen.hpp caen.cpp _CAENDigitizer.c _CAENDigitizer.h DPPQCDEvent.hpp DPPQCDEvent.cpp)
target_link_libraries(caen dl)

#file(GLOB DataHandlerHEADERS DataFormat.hpp DataHandler*.hpp uuid.hpp EventAccessor.hpp)
file(GLOB DataHandlerHEADERS DataFormat.hpp DataHandler.hpp DataWriterHDF5.hpp DataWriterText.hpp DataWriter.hpp DataWriterNetwork.hpp uuid.hpp EventAccessor.hpp  EventIterator.hpp)
#file(GLOB DataHandlerSOURCES DataHandler*.cpp uuid.cpp)
file(GLOB DataHandlerSOURCES uuid.cpp)
add_library(DataHandler ${DataHandlerHEADERS} ${DataHandlerSOURCES} Waveform.hpp)

add_executable(jadaq ${DataHandlerHEADERS} jadaq.cpp caen.hpp Configuration.cpp Configuration.hpp Digitizer.cpp Digitizer.hpp FunctionID.hpp FunctionID.cpp ini_parser.hpp StringConversion.cpp StringConversion.hpp CAENConf.cpp CAENConf.hpp trace.hpp interrupt.hpp container.hpp runno.cpp)
target_link_libraries(jadaq ${CAEN_LIB} caen DataHandler ${Boost_LIBRARIES} pthread ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})

# For jadaq-ds
# jadaq-ds now depends on caen and CAEN_LIB because of EventAccessor. Can we get rid of this dependency
#add_executable(jadaq-ds jadaq-ds.cpp ${DataHandlerHEADERS} NetworkReceive.cpp NetworkReceive.hpp trace.hpp interrupt.hpp)
#target_link_libraries(jadaq-ds ${CAEN_LIB} caen DataHandler ${HDF5_CXX_LIBRARIES} ${Boost_LIBRARIES} pthread)
