#!/bin/bash
######################################
##  ALL configuration is done here  ##

# BASE_NAME will be used to construct names of config-files and directories
# Intended to be used as the human readable name of the experiment 
BASE_NAME=TEST_STF_27_August_6digit

# Number of configurations to run thrue
N_CONFIG=1
# Runtime for each configuration in seconds 
RUN_TIME=60
# Amount of time that each file contains data for in seconds
SPLIT_TIME=10

# Data path
DATA_PATH=/data
# Configuration file path
CONFIG_PATH=${HOME}/jadaq/config
# path to binary
JADAQ=${HOME}/jadaq/build/jadaq

##        END of configuration       ##
#######################################

#######################################
##  DO NOT EDIT BELOW THIS LINE!!!   ##
#######################################


echo "================================================="

set_config()
{
    if [ ${N_CONFIG} == 1 ]
    then
        config=${CONFIG_PATH}/${BASE_NAME}.ini
    else
        config=${CONFIG_PATH}/${BASE_NAME}_${1}.ini
    fi    
}

base_path=${DATA_PATH}/${BASE_NAME}
set_path()
{
    if [ ${N_CONFIG} == 1 ]
    then
        path=${base_path}
    else
        path=${base_path}/`printf %03d ${i}`
    fi    
}

set_basename()
{
    if [ ${N_CONFIG} == 1 ]
    then
        basename=${BASE_NAME}-
    else
        basename=${BASE_NAME}-`printf %03d ${i}`-
    fi    
}

# check if executable exists 
if [ ! -x ${JADAQ} ]
then
    echo "The executeable \"${JADAQ}\" does not exist or does not have the correct permissions."
    exit 1
fi

# check all configuration files exist
for ((i=1;i<=${N_CONFIG};i++))
do
    set_config ${i}
    if [ ! -e ${config} ]
    then
        echo "Could not find the configuration file \"${config}\""
        exit 1
    fi
done
   
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
	echo -n "Press c to continue otherwise abort ... and press [ENTER] "
	read var_name
fi

if [ "$var_name" = "c" ]
then
	echo "================================================="		
	echo "${N_CONFIG} runs of ${RUN_TIME}s will start ..."
	echo "Data will be saved in: $path"
	datee=$(date +%F_%H-%M-%S)

	for ((i=1;i<=${N_CONFIG};i++))
    do
  		echo "================================================="
  		echo "===== Run ${i} of ${N_CONFIG}"
  		sleep .5
        set_config ${i}
        set_path ${i}
        set_basename ${i}
        mkdir -p $path
        cp ${config} ${path}
		COMMAND="${JADAQ} --hdf5 --time ${RUN_TIME} --split ${SPLIT_TIME} --config ${config} --path $path --basename ${basename} 2>&1 | tee -a ${path}/outputmessages_$datee.log"
        if [ ${1} ]
        then
            echo ${COMMAND}
        else
            eval ${COMMAND}
        fi
	done	
			
	echo "================================================="
	echo "${N_CONFIG} runs of ${RUN_TIME}s is complete     "
	echo "================================================="
	echo "================================================="
	echo "================================================="


else
	echo "Aborted..."
fi
