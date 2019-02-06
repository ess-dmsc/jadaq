# Running jadaq

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
and waveform events sent to as UDP to a host and port one would issue the commands:

```
./jadaq -N <ip-address> -P <udp-port> -e 1000 -s 'list waveform' mydigitizer.ini
```
in separate terminals.
