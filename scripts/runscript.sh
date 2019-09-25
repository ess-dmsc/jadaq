#!/bin/bash
######################################
##  ALL configuration is done here  ##

# BASE_NAME will be used to construct names of config-files and directories
# Intended to be used as the human readable name of the experiment 
BASE_NAME=CAEN-MB18-6D
# Configuration file path
CONFIG_PATH=${HOME}/jadaq/config
# path to binary
#JADAQ=${HOME}/jadaq-MB/build/jadaq
JADAQ=${HOME}/jadaq/build/jadaq

# Subfolder to save files in
DATA_PATH=/data/jadaq/${1:-STF}
# Prefix for filenames
PREFIX=${2:-goofy}
# Amount of time that each file contains data for in seconds
SPLIT_TIME=${3:-60} 
# Runtime for each configuration in seconds 
RUN_TIME=${4:-90000}

#if [ $# -ne 3 ]; then
#    echo Launch like ./runscript.sh PREFIX SPLIT_TIME RUN_TIME
#    echo Exiting...
#    exit 1
#else
#    PREFIX=pre
#    SPLIT_TIME=37
#    RUN_TIME=65
#fi

# Number of configurations to run thrue
N_CONFIG=1
# Print digitizer stats for STAT_TIME seconds
STAT_TIME=5

##        END of configuration       ##
#######################################

#######################################
##  DO NOT EDIT BELOW THIS LINE!!!   ##
#######################################


echo "================================================="

set_config()
{
	config=${CONFIG_PATH}/${BASE_NAME}.ini
}

base_path=${DATA_PATH}
set_path()
{
	path=${base_path}
}

set_basename()
{
    basename=${PREFIX}-${datee}_
}

# Check if run time is reasonable
if [ ${RUN_TIME} -le 30 ]; then
    echo ===== WARNING: Your run time is less than 30 seconds =====
fi

# Check if executable exists 
if [ ! -x ${JADAQ} ]
then
    echo "The executeable \"${JADAQ}\" does not exist or does not have the correct permissions."
    exit 1
fi

set_config
if [ ! -e ${config} ]; then
	echo "Could not find the configuration file \"${config}\""
	exit 1
fi

#check folder exist 
if [ -d "${base_path}" ]
then
    echo "${base_path} --> Exists"
else
    echo "${base_path} --> Folder does not exist --> It will be created"
    mkdir -p ${base_path}
fi

#check folder is empty
if [ -z "$(ls -A ${base_path})" ]
then
    echo "${base_path} --> it is empty"
    var_name="c"
else
    echo "${base_path} --> it is not empty"
   # echo -n "Press c to continue otherwise abort ... and press [ENTER] "
   # read var_name
    var_name="c"
fi

if [ "$var_name" = "c" ]
then
    echo "================================================="	
    echo "* The run of `printf %5d ${RUN_TIME}`s will start ...           *"
    echo "* File split every    `printf %5d ${SPLIT_TIME}`s                    *"
    echo "* Stats printed every `printf %5d ${STAT_TIME}`s                    *"
    echo "Data will be saved in: ${base_path}"
    datee=$(date +%y%m%d-%H%M%S)
	   
    echo "================================================="
    sleep .5
    set_config
    set_path
    set_basename
    mkdir -p $path
    cp ${config} ${path}/${BASE_NAME}.in.ini

    # Selet the appropriate run-mode (filewriting, forwarding to EFU, etc.)
#   COMMAND="${JADAQ} --hdf5 --time ${RUN_TIME} --split ${SPLIT_TIME} --stats ${STAT_TIME} --path $path --basename ${basename} --config ${config} --config_out ${path}/${BASE_NAME}.out.ini 2>&1 | tee -a ${path}/outputmessages_$datee.log"
#   COMMAND="${JADAQ} -T --time ${RUN_TIME} --split ${SPLIT_TIME} --stats ${STAT_TIME} --path $path --basename ${basename} --config ${config} --config_out ${path}/${BASE_NAME}.out.ini 2>&1 | tee -a ${path}/outputmessages_$datee.log"
   COMMAND="${JADAQ} -N 192.168.0.58 -P 9000 --time ${RUN_TIME} --stats ${STAT_TIME} --path $path --basename ${basename} --config ${config} --config_out ${path}/${BASE_NAME}.out.ini 2>&1 | tee -a ${path}/outputmessages_$datee.log"

    #if [ ${1} ]; then
    #    echo ${COMMAND}
    #	else
    #    eval ${COMMAND}
    #fi
    echo ==== Running: ${COMMAND}
    eval ${COMMAND}

    echo "================================================="
    echo "One run of ${RUN_TIME}s is complete     "
    echo "================================================="
    echo "================================================="
    echo "================================================="
    
    
else
    echo "Aborted..."
fi
