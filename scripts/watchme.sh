#!/bin/bash
# run it from another terminal
# change the email address to your address

#######################################
#path="/data/test27log"
#######################################

while true
do
        # search for the jadaq process, return the process ID or 0 if does not exist
	lsal=$(pidof jadaq)
        
	# if exist print, if not print the time and date and send email to user.
        if [ $lsal ] 
	then
           echo "jadaq is running.... " $lsal
		
		#pid=`grep -i ERROR ${path}/outputmessages_*.log`
		#if [ "$pid"="" ] 
		#then
		#	echo "no errors detected..."
		#else		 	
   		#	dateE2=$(date +%F_%H-%M-%S)
		#	title2="jadaq error found at:"
		#	echo -e $title2 "\n" $dateE2  "\n" $pid > temp2
		#	mail -s "jadaq error!!!" francesco.piscitelli@esss.se < temp2
		#	rm temp2
		#fi

        else
	   echo "jadaq is not running!!!"
	   dateE=$(date)
	   #dateA=`TZ=America/New_York date`

	   # print information on the terminal
	   echo "Stopped at:" $dateE
	   #echo 'Stops at:' $dateA
           
	   # craete a temp file with the neccesary information, you can add whatever you want.... 
	   title="jadaq stopped at:"
           #echo -e $title "\n" $dateE "\n" $dateA > temp
           echo -e $title "\n" $dateE  > temp
	
	   # send email
	   mail -s "jadaq is not running!!!" francesco.piscitelli@esss.se < temp 

	   # remove temp file
	   rm temp
	  # exit 0
	fi
 
	sleep 10
done 

