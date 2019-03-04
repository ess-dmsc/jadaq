# JADAQ

Just Another DAQ (work in progress) for reading selected Caen
digitizers for ESS Detector Group.

- [Further documentation](documentation/README.md)

## Getting Started

### Prerequisites
To follow these instructions you need the following software
 * git
 * cmake
 * boost-devel
 * hdf5-devel (>1.10)
 * python
 * gcc/g++ (5.3 or later)

For details on the prerequites and installation see [documentation](documentation/install.md)

### Installing
As usual for a CMake project:
```
> cmake <path-to-source>
> make
```

To specify building with Conan for dependency management and with custom location of
Caen libraries
```
> cmake <path-to-source> -DCAEN_ROOT=<path-to-caenlibs> -DCONAN=AUTO
> make
```


## Authors

* **Troels Blum** - *Initial work* - [troels-blum](https://github.com/tblum)

See also the list of [contributors](https://github.com/ess-dmsc/jadaq/graphs/contributors) who
participated in this project.
