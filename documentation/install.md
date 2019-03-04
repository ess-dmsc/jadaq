# Installation details

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
git clone https://github.com/ess-dmsc/jadaq.git jadaq
```
or if you have ssh git access you might prefer:
```
git clone git@github.com:ess-dmsc/jadaq.git jadaq
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
