#!/bin/bash
#
# Take the list of all CAEN functions implemented in CAENDigitizer and
# show the names of the ones not yet implemented in jadaq caen.hpp

# Default paths - can be overridden in command line
JADAQBASE=$(dirname "$0")
CAENDIGITIZERBASE="/usr/local"
if [ $# -gt 0 ]; then
    CAENDIGITIZERBASE="$1"
    echo "looking for CAENDigitizer header in $CAENDIGITIZERBASE"
fi

JADAQCAENHEADER="$JADAQBASE/caen.hpp"
CAENDIGITIZERHEADER="$CAENDIGITIZERBASE/include/CAENDigitizer.h"
CAENFUNCTIONLIST="$JADAQBASE/CAEN-functions.txt"

if [[ ! -f "$CAENFUNCTIONLIST" ]]; then
    if [[ ! -f "$CAENDIGITIZERHEADER" ]]; then
	echo "ERROR: no such CAENDigitizer header: $CAENDIGITIZERHEADER"
	exit 1
    fi
    echo "building list of CAENDigitizer functions from $CAENDIGITIZERHEADER"
    egrep '^CAEN_DGTZ_ErrorCode CAENDGTZ_API CAEN_DGTZ_' $CAENDIGITIZERHEADER | \
	grep -v DEPRECATED | \
	sed 's/CAEN_DGTZ_ErrorCode CAENDGTZ_API //g;s/(.*//g' > $CAENFUNCTIONLIST
fi

for i in $(cat $CAENFUNCTIONLIST); do
    grep -q $i $JADAQCAENHEADER || echo "$i is missing in $JADAQCAENHEADER"
done

exit 0
