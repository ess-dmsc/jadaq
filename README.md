# JADAQ

Just Another DAQ (work in progress) for reading selected Caen
digitizers for ESS Detector Group.

- [Further documentation](documentation/README.md)

## Getting Started

### Prerequisites
To build and run this software the following dependencies are required

 * [**Conan**](https://conan.io) The conan script has to be available in the current ``$PATH``.
 * [**CMake**](https://cmake.org) At least CMake version 3.2 is required. Some packages that requires more recent versions of CMake will download this as a dependency.
 * [**bash**](https://www.gnu.org/software/bash/) For properly setting paths to the conan provided dependencies.
 * A recent C/C++ compiler with support for C++11.

Conan is used to download dependencies. For conan to know where the dependencies can be downloaded from, package repositories must be added by running the instructions from the [conan-configuration](https://github.com/ess-dmsc/conan-configuration) repository.

For details on the prerequites and installation see [documentation](documentation/install.md)

### Installing
To specify building with Conan for dependency management and with custom location of
Caen libraries
```
> cmake <path-to-source> -DCAEN_ROOT=<path-to-caenlibs> -DCONAN=AUTO
> make
```

If you prefer to run conan manually the procedure is
```
> conan install <path-to-source> --build=missing
> cmake <path-to-source> -DCAEN_ROOT=<path-to-caenlibs> -DCONAN=MANUAL
> make
```

## Contributing

Please read the [CONTRIBUTING.md](CONTRIBUTING.md) file for details on our code of conduct and the process for submitting pull requests to us.

## Authors

* Troels Blum
* Jonas Bardino
* Hanno Perrey

See also the [list of contributors](https://github.com/ess-dmsc/jadaq/graphs/contributors) on Github.
