# Debugging

## Debugging jumps in DPP timestamps
We have seen occasional jumps in the resulting event timestamps. It
looks like the acquisition can't keep up if the events arrive often
enough and aggregates aren't tuned to catch them.
In that case it may be helpful to use the hdf5dump.py and chktimewarp.py
scripts. First run the acquisition with events sent to the hdf5writer as
described in the previous sections. This will save the events in out.h5
by default. Then make a simple flat text dump of the events from all
channels and sorted after timestamp rather than receiving order:

```
python hdf5dump.py out.h5 debug-warp.txt 0 1
```
Now you can use chktimewarp.py to analyse the timestamps for unexpected
large jumps:

```
python chktimewarp.py debug-warp.txt
```
If the run was without any such jumps the output is something like

```
Parsing simple dump in debug-warp.txt
Checking 1025 parsed entries for time warps
Found 0 of 1025 time jumps much bigger than 62500
```
Otherwise the jumps will be listed as in

```
Parsing simple dump in warp.txt
Checking 1006 parsed entries for time warps
time warp of 937496 (1507040 vs 569544) for entry 10
...
time warp of 687498 (136444111 vs 135756613) for entry 1005
Found 99 of 1006 time jumps much bigger than 62499
```

If you have python matplotlib installed the script will also plot the
timestamps.


## Debugging the CAEN layer

debugCAENComm is a layer that can be inserted between the CAENDigitizer library
and the CAENComm library for debugging purposes. It will print out which
functions are being called in CAENComm including parameters and return values

Usage:
LD_PRELOAD=$PWD/libdebugCAENComm.so ./jadaq
