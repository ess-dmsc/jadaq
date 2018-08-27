#!/bin/bash
######################################
N=9999
Duration=6
path="/data/test21aug_bis"
#######################################

echo "================================================="		

#check folder exist 
if [ -d "$path" ]; then
	echo "$path --> Exists"
else
	echo "$path --> Folder does not exist --> It will be created"
	mkdir $path
fi
 
#check folder is empty
if [ -z "$(ls -A $path)" ]; then
	echo "$path --> it is empty"
	var_name="c"
	else
	echo "$path --> it is not empty"
	echo -n "Press c to continue otherwise abort ... and press [ENTER] "
	read var_name
fi

if [ "$var_name" = "c" ]; then
	echo "================================================="		
	echo "$N runs of $Duration s will start ..."
	echo "Data will be saved in: $path"
	datee=$(date +%F_%H-%M-%S)

	for ((i=1;i<=$N;i++)); do
  		echo "================================================="
  		echo "===== Run $i of $N"
  		sleep .5
		#  /home/mb/jadaq/build/jadaq -t $Duration -H /home/mb/jadaq/config/config2digit_STF_FP2307.ini 
		/home/mb/jadaq/build/jadaq -t $Duration -H /home/mb/jadaq/config/config2digit_STF_21aug.ini -p $path 2>&1 | tee -a ${path}/outputmessages_$datee.log
 		# /home/mb/jadaq/build/jadaq -t $Duration -H /home/mb/jadaq/config/configtemp.ini
	done	
			
	echo "================================================="
	echo "$N runs of $Duration s completed."
	echo "================================================="
	echo "================================================="
	echo "================================================="

 	#for i in $( ls -1 $dataPath ); do
	#  root -b -q -l `printf "readBin.C(\"%s\")" "${dataPath}${i}"`

else
	echo "Aborted..."
fi
