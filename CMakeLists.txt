
cmake_minimum_required(VERSION 3.2)
# cmake_policy(VERSION 3.12)

project(jadaq)

#=============================================================================
# Augment CMake with our custom scripts
#=============================================================================
set(EXTRA_MODULES_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

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

Find_Package(CAEN REQUIRED)
include_directories(${CAEN_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -flto -O3")
set(CMAKE_CXX_STANDARD 11)
set(ACCEPED_WARNINGS "-Wno-unused-function -Wno-unused-variable -Wcast-align")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall ${ACCEPED_WARNINGS} -Werror" )
set(CMAKE_C_STANDARD 99)

if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.3)
    message(FATAL_ERROR "Require at least g++-5.3: On CentOS-7 use Devtoolset > 4")
endif()

find_package(HDF5 1.10 REQUIRED COMPONENTS C CXX HL)
include_directories(${HDF5_INCLUDE_DIRS})

find_package(Boost COMPONENTS system filesystem thread program_options REQUIRED )

set(jadaq_SRC
  src/Configuration.cpp
  src/Digitizer.cpp
  src/DPPQDCEvent.cpp
  src/runno.cpp
  src/FunctionID.cpp
  src/StringConversion.cpp
  src/caen.cpp
  src/jadaq.cpp
)
set(jadaq_INC
  src/Configuration.hpp
  src/DataFormat.hpp
  src/DataHandler.hpp
  src/DataWriter.hpp
  src/DataWriterNetwork.hpp
  src/DataWriterHDF5.hpp
  src/Digitizer.hpp
  src/DPPQDCEvent.hpp
  src/EventIterator.hpp
  src/FunctionID.hpp
  src/StringConversion.hpp
  src/Waveform.hpp
  src/caen.hpp
  src/container.hpp
  src/ini_parser.hpp
  src/interrupt.hpp
  src/xtrace.h
  src/timer.h
)

add_executable(jadaq ${jadaq_INC} ${jadaq_SRC})

target_link_libraries(jadaq ${CAEN_LIBRARIES} pthread)

target_link_libraries(jadaq ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})

if(${CONAN} MATCHES "AUTO")
  target_link_libraries(jadaq Boost::filesystem Boost::system Boost::thread Boost::program_options)
else()
  target_link_libraries(jadaq ${Boost_LIBRARIES})
endif()
