# JADAQ
jadaq - Just Another DAQ - very much work in progress


## build and runtime dependencies
To follow these instructions you need the following software
 * git
 * cmake
 * boost-devel
 * hdf5-devel (>1.10)
 * python
 * gcc/g++ (5.3 or later)

### Building HDF5 from source
If your distribution does not ship with a current version of HDF5, you can build
it directly from sources: https://www.hdfgroup.org/downloads/hdf5/source-code/

```
cd hdf5/source
mkdir build
cd build
../configure --prefix=$HOME/local/hdf5 --enable-cxx
```

## Set up
Clone the project from github to a local jadaq folder:
```
git clone https://github.com/tblum/jadaq.git jadaq
```
or if you have ssh git access you might prefer:
```
git clone git@github.com:tblum/jadaq.git jadaq
```

Make sure the build dependencies are installed. On CentOS/RHEL 7 it can
be done with a command like:

```
yum install git cmake boost-devel hdf5-devel python
```

Since CentOS/RHEL does not provide a recent compiler suite it is
necessary to install and use the devtools-7 package
instead. Instructions are available at:

https://www.softwarecollections.org/en/scls/rhscl/devtoolset-7/

In that case you have to either explicitly launch a devtools-7 environment with

```
scl enable devtoolset-7 bash
```
or set it up permanently in your shell configuration.

Now enter the cloned jadaq dir and run cmake to prepare building:

```
cd jadaq
mkdir build
cd build
cmake ..
```

If you want to specify an alternative path to certain libraries, you can append
the following switches to the `cmake` call:

`-DHDF5_ROOT=$HOME/local/hdf5`

When `cmake` has run successfully, you should be able to just use 'make' to build the binaries:

```
make
```
this will create a.o. the jadaq binary in the current directory.

## Running
Once built you can run the binaries. The main application is 'jadaq' and
you can get information about it and command line option by running:

```
./jadaq -h
```

Typically you want to run it with at least a configuration file as
argument:

```
./jadaq test.ini
```
 where test.ini could be something as simple as

```
[digi1]
USB=0
```
if you want to use a digitizer on the first USB slot.

If dealing with VME on address 0x11130000 you would instead use

```
[digi1]
USB=0
VME=0x11130000
```

You can run it once to get a complete configuration file as output that
you can then tweak in further runs.

```
./jadaq -t 1 test.ini
```
then you can take the resulting test.ini.out and rename to
mydigitizer.ini and tweak to your desires before running again with:

```
./jadaq mydigitizer.ini
```

To make a run until at least 1000 events are received and with both list
and waveform events sent to the hdf5writer one would issue the commands:

```
./hdf5writer
```
and

```
./jadaq -a 127.0.0.1 -e 1000 -s 'list waveform' mydigitizer.ini
```
in separate terminals.

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
