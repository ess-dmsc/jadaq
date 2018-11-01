
cmake_minimum_required(VERSION 3.2)

project(jadaq)

if(CAEN_PATH)
  message(STATUS ">>>> Using custom location, ${CAEN_PATH}, for Caen libs")
  set(CAEN_INC_PATH ${CAEN_PATH}/include)
  set(CAEN_LIB_PATH ${CAEN_PATH}/lib)
else()
  message(STATUS ">>>> Using default location for Caen libs")
  set(CAEN_INC_PATH $ENV{HOME}/include)
  set(CAEN_LIB_PATH $ENV{HOME}/lib)
endif()

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
  EventAccessor.hpp
  EventIterator.hpp
  FunctionID.hpp
  StringConversion.hpp
  Timer.hpp
  Waveform.hpp
  caen.hpp
  container.hpp
  ini_parser.hpp
  interrupt.hpp
  xtrace.h
)

add_executable(jadaq ${jadaq_INC} ${jadaq_SRC})

if (JENKINS)
  target_link_libraries(jadaq ${CAEN_LIB} ${Boost_LIBRARIES} pthread)
else()
  target_link_libraries(jadaq ${CAEN_LIB} ${Boost_LIBRARIES} pthread)
endif()
