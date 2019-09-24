# Installation details

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
yum install git python
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
