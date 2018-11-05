
cmake_minimum_required(VERSION 3.2)

project(jadaq)

#=============================================================================
# Augment CMake with our custom scripts
#=============================================================================
set(EXTRA_MODULES_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake)

#=============================================================================
# Conan
#=============================================================================
SET(CONAN_PROFILE "default" CACHE STRING "Name of conan profile to use, uses default by default")
SET(CONAN "DISABLED" CACHE STRING "conan options AUTO (conan must be in path), MANUAL (expects conanbuildinfo.cmake in build directory) or DISABLE")
if(${CONAN} MATCHES "AUTO")
  include(${EXTRA_MODULES_DIR}/conan.cmake)
  conan_cmake_run(CONANFILE conanfile.txt
    PROFILE ${CONAN_PROFILE}
    BASIC_SETUP NO_OUTPUT_DIRS KEEP_RPATHS
    BUILD_TYPE "None"
    BUILD outdated)
elseif(${CONAN} MATCHES "MANUAL")
  if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    conan_basic_setup(NO_OUTPUT_DIRS SKIP_RPATH)
  else()
    MESSAGE(FATAL_ERROR "CONAN set to MANUAL but no file named conanbuildinfo.cmake found in build directory")
  endif()
elseif(NOT ${CONAN} MATCHES "DISABLE")
  MESSAGE(FATAL_ERROR "Unrecognised option for CONAN (${CONAN}), use AUTO, MANUAL or DISABLE")
endif()

if(${CONAN} MATCHES "AUTO")
  set(Boost_USE_STATIC_LIBS OFF)
  set(Boost_USE_MULTITHREADED ON)
  set(Boost_USE_STATIC_RUNTIME OFF)
endif()

if(CAEN_PATH)
  message(STATUS ">>>> Using custom location, ${CAEN_PATH}, for Caen libs")
  set(CAEN_INC_PATH ${CAEN_PATH}/include)
  set(CAEN_LIB_PATH ${CAEN_PATH}/lib)
else()
  message(STATUS ">>>> Using default location for Caen libs")
  set(CAEN_INC_PATH $ENV{HOME}/include)
  set(CAEN_LIB_PATH $ENV{HOME}/lib)
endif()


set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -flto -O3")
set(CMAKE_CXX_STANDARD 11)
set(ACCEPED_WARNINGS "-Wno-unused-function")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fpermissive -Wall ${ACCEPED_WARNINGS}" )
set(CMAKE_C_STANDARD 99)

if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.3)
    message(FATAL_ERROR "Require at least g++-5.3: On CentOS-7 use Devtoolset-4")
endif()

find_package(HDF5 1.10 REQUIRED COMPONENTS C CXX HL)
include_directories(${HDF5_INCLUDE_DIRS})

find_path(CAEN_INCLUDE_DIR NAMES CAENDigitizer.h   HINTS   ${CAEN_INC_PATH})
find_library(CAEN_LIB NAMES CAENDigitizer CAENComm HINTS   ${CAEN_LIB_PATH})
include_directories(${CAEN_INCLUDE_DIR})

find_package(Boost COMPONENTS system filesystem thread program_options REQUIRED )

set(jadaq_SRC
  _CAENDigitizer.c
  Configuration.cpp
  Digitizer.cpp
  DPPQCDEvent.cpp
  FunctionID.cpp
  StringConversion.cpp
  caen.cpp
  jadaq.cpp
)
set(jadaq_INC
  _CAENDigitizer.h
  Configuration.hpp
  DataFormat.hpp
  DataHandler.hpp
  DataWriter.hpp
  DataWriterNetwork.hpp
  Digitizer.hpp
  DPPQCDEvent.hpp
  EventIterator.hpp
  FunctionID.hpp
  StringConversion.hpp
  Waveform.hpp
  caen.hpp
  container.hpp
  ini_parser.hpp
  interrupt.hpp
  xtrace.h
  timer.h
)

add_executable(jadaq ${jadaq_INC} ${jadaq_SRC})

target_link_libraries(jadaq ${CAEN_LIB} pthread)

if(${CONAN} MATCHES "AUTO")
  target_link_libraries(jadaq Boost::filesystem Boost::system Boost::thread Boost::program_options)
else()
  target_link_libraries(jadaq ${Boost_LIBRARIES})
endif()
