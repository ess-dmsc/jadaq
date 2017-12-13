#!/usr/bin/python

"""Simple hdf5 dump from python, which reads HDF5 files in SWMR mode and
outputs the contents to a simple column format text file sorted by global+local
time.
"""

import os
import sys
import time
import h5py

def extract_listelem_struct(serial, name, data):
    """Helper to print list elements on compound form"""
    return "%16s %8s %8s %8s" % (data[0]['localTime_name'], serial,
                            data[0]['channel_name'], data[0]['adcValue_name'])

def extract_listelem_flat(serial, name, data):
    """Helper to print list elements on flat array form"""
    return "%16s %8s %8s %8s" % (data[0][0], serial, data[0][1], data[0][2])

def extract_waveformelem_struct(serial, name, data):
    """Helper to print waveform elements on compound form"""
    return "%16s %8s %8s %s" % (data[0]['localTime_name'], serial,
                            data[0]['channel_name'], ' '.join(data[0]['waveform_name']))

def dump_entries(serial, root, out_fd):
    """Write out the contents of root and nested values in simple columns in
    file outpath.
    """
    if not serial and root.name != "/":
        serial = root.name.split("_", 1)[1]
    for (key, val) in root.items():
        line = None
        try:
            # We extract from compound element fields based on the DataFormat
            # struct and the corresponding names defined in the hdf5writer.
            if key.startswith('list-'):
                if val.shape[0] == 1:
                    line = extract_listelem_struct(serial, key, val)
                else:
                    line = extract_listelem_flat(serial, key, val)
            elif key.startswith('waveform-'):
                line = extract_waveformelem_struct(serial, key, val)
            if line:
                out_fd.write("%s\n" % line)
            # Keep following until we hit a leaf node
            dump_entries(serial, root[key], out_fd)
        except:
            # just ignore leaf entries
            pass
    
if __name__ == '__main__':
    inpath = 'out.h5'
    outpath = 'out.txt'
    if sys.argv[1:]:
        inpath = sys.argv[1]
    if sys.argv[2:]:
        outpath = sys.argv[2]
    try:
        h5file = h5py.File(inpath, 'r', libver='latest', swmr=True)
    except Exception, exc:
        print "ERROR: failed to open %s in SWMR-mode: %s" % (inpath, exc)
        print "Are you sure it is a HDF5 file created/opened in SWMR mode?"
        sys.exit(1)
    root = h5file["/"]
    print "Dumping contents as simple text columns in %s" % outpath
    try:
        out_fd = open(outpath, 'w')
        dump_entries(None, root, out_fd)
        out_fd.close()
        print "Dumped contents to %s" % outpath
    except Exception, exc:
        print "ERROR: failed to dump to %s: %s" % (outpath, exc)

