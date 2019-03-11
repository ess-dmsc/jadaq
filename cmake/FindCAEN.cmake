# - Try to find CAENDigitizer library
# Once done this will define
#  CAEN_FOUND - System has CAENDigitizer libs
#  CAEN_INCLUDE_DIRS - The CAENDigitizer libs include directories
#  CAEN_LIBRARIES - The libraries needed to use CAENDigitizer libs
#  CAEN_DEFINITIONS - Compiler switches required for using CAENDigitizer libs

# in case we are shipping a version in the /extern/ subdirectory of our source distribution
file(GLOB_RECURSE extern_file ${PROJECT_SOURCE_DIR}/extern/*CAENDigitizerlib.h)
if (extern_file)
  get_filename_component(extern_lib_path ${extern_file} PATH)
#  MESSAGE(STATUS "Found CAENDigitizer library in 'extern' subfolder: ${extern_lib_path}")
endif(extern_file)


find_path(CAEN_INCLUDE_DIR CAENDigitizer.h
  HINTS
  ${CAEN_ROOT}/include
  $ENV{CAEN_ROOT}/include
  /usr/local/include
  /usr/include
  /opt/local/include
  $ENV{HOME}/include
  ${extern_lib_path}/include
PATH_SUFFIXES CAENDigitizerLib )

# library might be installed in either or both 32/64bit, need to figure out which one to use
if(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
  # using 32 bit compiler
  find_library(CAENDigitizer_LIBRARY NAMES CAENDigitizer
    HINTS
    ${CAEN_ROOT}/lib
    $ENV{CAEN_ROOT}/lib
    /usr/local/lib
    /usr/lib
    /opt/local/lib
    $ENV{HOME}/lib
    ${extern_lib_path}/lib/x86
    )
else(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
  # using 64 bit compiler
  find_library(CAENDigitizer_LIBRARY NAMES CAENDigitizer
    HINTS
    # focus on default 64-bit paths used e.g. by RedHat systems
    ${CAEN_ROOT}/lib64
    $ENV{CAEN_ROOT}/lib64
    /usr/local/lib64
    /usr/lib64
    /opt/local/lib64
    $ENV{HOME}/lib64
    ${extern_lib_path}/lib/x64
    # else, try the usual suspects:
    ${CAEN_ROOT}/lib
    $ENV{CAEN_ROOT}/lib
    /usr/local/lib
    /usr/lib
    /opt/local/lib
    $ENV{HOME}/lib
    )
endif()

# library might be installed in either or both 32/64bit, need to figure out which one to use
if(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
  # using 32 bit compiler
  find_library(CAENComm_LIBRARY NAMES CAENComm
    HINTS
    ${CAEN_ROOT}/lib
    $ENV{CAEN_ROOT}/lib
    /usr/local/lib
    /usr/lib
    /opt/local/lib
    $ENV{HOME}/lib
    ${extern_lib_path}/lib/x86
    )
else(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
  # using 64 bit compiler
  find_library(CAENComm_LIBRARY NAMES CAENComm
    HINTS
    # focus on default 64-bit paths used e.g. by RedHat systems
    ${CAEN_ROOT}/lib64
    $ENV{CAEN_ROOT}/lib64
    /usr/local/lib64
    /usr/lib64
    /opt/local/lib64
    $ENV{HOME}/lib64
    ${extern_lib_path}/lib/x64
    # else, try the usual suspects:
    /usr/local/lib
    /usr/lib
    /opt/local/lib
    $ENV{HOME}/lib
    )
endif()

# resolve symlinks
get_filename_component(CAENDigitizer_LIBRARY ${CAENDigitizer_LIBRARY} REALPATH)
get_filename_component(CAENComm_LIBRARY ${CAENComm_LIBRARY} REALPATH)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CAENDigitizer_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args("CAEN" DEFAULT_MSG
                                  CAEN_INCLUDE_DIR CAENDigitizer_LIBRARY CAENComm_LIBRARY)

if(CAEN_FOUND)
  if(NOT CAEN_FIND_QUIETLY)
    message(STATUS "Found the following CAEN libraries:")
    message(STATUS "  CAEN Comm: ${CAENComm_LIBRARY}")
    message(STATUS "  CAEN Digitizer: ${CAENDigitizer_LIBRARY}")
    message(STATUS "  CAEN includes: ${CAEN_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced( CAEN_INCLUDE_DIR CAENDigitizer_LIBRARY CAENComm_LIBRARY)

set(CAEN_LIBRARIES ${CAENDigitizer_LIBRARY} ${CAENComm_LIBRARY} )
set( CAEN_INCLUDE_DIRS ${CAEN_INCLUDE_DIR} )
