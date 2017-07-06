jadaq - Just Another DAQ - very much work in progress


debugCAENComm is a layer that can be inserted between the CAENDigitizer library
and the CAENComm library for debugging purposes. It will print out which
functions are being called in CAENComm including parameters and return values

Usage:
LD_PRELOAD=$PWD/libdebugCAENComm.so ./jadaq
