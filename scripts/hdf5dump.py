#!/usr/bin/python

"""Simple hdf5 dump from python, which reads HDF5 files in SWMR mode and
outputs the contents to a simple column format text file sorted by global+local
time.
"""

import os
import sys
import time
import h5py

def extract_listelem_struct(serial, name, data, globaltime):
    """Helper to print list elements on compound form"""
    line = ""
    if with_globaltime:
        line = "%16s " % globaltime
    line += "%16s %8s %8s %8s\n" % (data[0]['localTime_name'], serial,
                                    data[0]['channel_name'],
                                    data[0]['adcValue_name'])
    return line

def extract_listelem_flat(serial, name, data, globaltime):
    """Helper to print list elements on flat array form"""
    line = ""
    if with_globaltime:
        line = "%16s " % globaltime
    line += "%16s %8s %8s %8s\n" % (data[0][0], serial, data[0][1], data[0][2])
    return line

def extract_waveformelem_struct(serial, name, data, globaltime):
    """Helper to print waveform elements on compound form"""
    line = ""
    if with_globaltime:
        line = "%16s " % globaltime
    line += "%16s %8s %8s %s\n" % (data[0]['localTime_name'], serial,
                                   data[0]['channel_name'],
                                   ' '.join(data[0]['waveform_name']))
    return line

def dump_entries(serial, root, all_lines, with_globaltime=False):
    """Write out the contents of root and nested values in simple columns in
    file outpath.
    """
    globaltime = None
    if not serial and root.name != "/":
        serial = root.name.split("_", 1)[1]
    if with_globaltime and root.name.count("/") == 2:
        globaltime = root.name.split("/", 2)[2]
    for (key, val) in root.items():
        line = None
        try:
            # We extract from compound element fields based on the DataFormat
            # struct and the corresponding names defined in the hdf5writer.
            if key.startswith('list-'):
                if val.shape[0] == 1:
                    line = extract_listelem_struct(serial, key, val,
                                                   globaltime)
                else:
                    line = extract_listelem_flat(serial, key, val,
                                                 globaltime)
            elif key.startswith('waveform-'):
                line = extract_waveformelem_struct(serial, key, val,
                                                   globaltime)
            if line:
                all_lines.append(line)
            # Keep following until we hit a leaf node
            dump_entries(serial, root[key], all_lines, with_globaltime)
        except:
            # just ignore leaf entries
            pass
    return all_lines
    
if __name__ == '__main__':
    inpath = 'out.h5'
    outpath = 'out.txt'
    with_globaltime = False
    sort_lines = False
    if sys.argv[1:]:
        inpath = sys.argv[1]
    if sys.argv[2:]:
        outpath = sys.argv[2]
    if sys.argv[3:]:
        with_globaltime = (sys.argv[3][0].lower() in ('y', '1', 't'))
    if sys.argv[4:]:
        sort_lines = (sys.argv[4][0].lower() in ('y', '1', 't'))
    
    try:
        h5file = h5py.File(inpath, 'r', libver='latest', swmr=True)
    except Exception, exc:
        print "ERROR: failed to open %s in SWMR-mode: %s" % (inpath, exc)
        print "Are you sure it is a HDF5 file created/opened in SWMR mode?"
        sys.exit(1)
    root = h5file["/"]
    all_lines = []
    print "Dumping contents as simple text columns in %s" % outpath
    try:
        out_fd = open(outpath, 'w')
        dump_entries(None, root, all_lines, with_globaltime)
        if sort_lines:
            all_lines.sort()
        out_fd.writelines(all_lines)
        out_fd.close()
        print "Dumped contents to %s" % outpath
    except Exception, exc:
        print "ERROR: failed to dump to %s: %s" % (outpath, exc)

