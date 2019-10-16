\#!/bin/bash
# Configuration file for /scripts/runscript.sh

# Jadaq binary path
JADAQ_PATH=${JADAQ_DIR}/build/jadaq
# Configuration file path
CONFIG_PATH=${JADAQ_DIR}/config
# Configuration file
CONFIG=CAEN-MB18-6D-lowTh

# Standard mode (0 for JADAQ filewriting, 1 to forward to EFU)
_EFU_MODE=1

# Data folder - where to save files (JADAQ FileWriting only!)
DATA_PATH=/data
SUBFOLDER=dump
PREFIX=Test

# Standard time intervals
SPLIT_TIME=60
RUN_TIME=900000
STAT_TIME=5

# EFU settings
_EFU_IP=192.168.0.58
_EFU_PORT=9000
