= JADAQ =
jadaq - Just Another DAQ - very much work in progress


== Install build and runtime dependencies ==
git
cmake
boost-devel
hdf5-devel
python
gcc/g++ (5.3 or later)


== Set up ==
Clone the project from github:
´´´
git clone git@github.com:tblum/jadaq.git jadaq
´´´

Make sure the build dependencies are installed. On CentOS/RHEL 7 it can
be done with a command like:
´´´
yum install git cmake boost-devel hdf5-devel python
´´´

Since CentOS/RHEL does not provide a recent compiler suite it is
necessary to install and use the devtools-4 package
instead. Instructions are available at:
https://www.softwarecollections.org/en/scls/rhscl/devtoolset-4/

In that case you have to either explicitly launch a devtools-4 environment with
´´´
scl enable devtoolset-4 bash
´´´
or set it up permanently in your shell configuration.

Now enter the cloned jadaq dir and run cmake to prepare building:
´´´
cd jadaq
mkdir build
cd build
cmake ..
´´´

From now on you should be able to just use 'make' to build the binaries:
´´´
make
´´´
this will create a.o. the jadaq binary in the current directory.

== Running ==
Once built you can run the binaries. The main application is 'jadaq' and
you can get information about it and command line option by running:
´´´
./jadaq -h
´´´

Typically you want to run it with at least a configuration file as
argument:
´´´
./jadaq test.ini
´´´
 where test.ini could be something as simple as
´´´
[digi1]
USB=0
´´´
if you want to use a digitizer on the first USB slot.

If dealing with VME on address 0x11130000 you would instead use
´´´
[digi1]
USB=0
VME=0x11130000
´´´

You can run it once to get a complete configuration file as output that
you can then tweak in further runs.
´´´
./jadaq -t 1 test.ini
´´´
then you can take the resulting test.ini.out and rename to
mydigitizer.ini and tweak to your desires before running again with:
´´´
./jadaq mydigitizer.ini
´´´

To make a run until at least 1000 events are received and with both list
and waveform events sent to the hdf5writer one would issue the commands:
´´´
./hdf5writer
´´´
and
´´´
./jadaq -a 127.0.0.1 -e 1000 -s 'list waveform' mydigitizer.ini
´´´
in separate terminals.

== Debugging jumps in DPP timestamps ==
We have seen occasional jumps in the resulting event timestamps. It
looks like the acquisition can't keep up if the events arrive often
enough and aggregates aren't tuned to catch them.
In that case it may be helpful to use the hdf5dump.py and chktimewarp.py
scripts. First run the acquisition with events sent to the hdf5writer as
described in the previous sections. This will save the events in out.h5
by default. Then make a simple flat text dump of the events from all
channels and sorted after timestamp rather than receiving order: 
´´´
python hdf5dump.py out.h5 debug-warp.txt 0 1
´´´
Now you can use chktimewarp.py to analyse the timestamps for unexpected
large jumps:
´´´
python chktimewarp.py debug-warp.txt
´´´
If the run was without any such jumps the output is something like
´´´
Parsing simple dump in debug-warp.txt
Checking 1025 parsed entries for time warps
Found 0 of 1025 time jumps much bigger than 62500
´´´
Otherwise the jumps will be listed as in 
´´´
Parsing simple dump in warp.txt
Checking 1006 parsed entries for time warps
time warp of 937496 (1507040 vs 569544) for entry 10
...
time warp of 687498 (136444111 vs 135756613) for entry 1005
Found 99 of 1006 time jumps much bigger than 62499
´´´

If you have python matplotlib installed the script will also plot the
timestamps.


== Debugging the CAEN layer ==

debugCAENComm is a layer that can be inserted between the CAENDigitizer library
and the CAENComm library for debugging purposes. It will print out which
functions are being called in CAENComm including parameters and return values

Usage:
LD_PRELOAD=$PWD/libdebugCAENComm.so ./jadaq
