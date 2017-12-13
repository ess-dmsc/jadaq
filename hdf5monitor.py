#!/usr/bin/python

"""Simple hdf5 dumper from python, which reads HDF5 files in SWMR mode"""

import os
import sys
import time
import h5py

# Names of struct fields in the order of legacy flat array fields
listelem_fields = ['channel', 'localTime', 'adcValue', 'extendTime']
waveformelem_fields = ['channel', 'localTime', 'extendTime', 'waveform']

def show_listelem_struct(name, data):
    """Helper to print list elements on compound form"""
    print "Element %s:" % name
    for field in listelem_fields:
        key = "%s_name" % field
        print "    %s %d" % (field, data[0][key])

def show_listelem_flat(name, data):
    """Helper to print list elements on flat array form"""
    print "Element %s:" % name
    for (field, index) in zip(listelem_fields, range(len(listelem_fields))):    
        print "    %s %d" % (field, data[index])

def show_waveformelem_struct(name, data):
    """Helper to print waveform elements on compound form"""
    print "Element %s:" % name
    for field in waveformelem_fields:
        key = "%s_name" % field
        print "    %s %d" % (field, data[0][key])

def show_entries(root):
    """Print out the contents of root and recursively apply to nested items"""
    
    for (key, val) in root.items():
        try:
            # We print out compound element fields based on the DataFormat
            # struct and the corresponding names defined in the hdf5writer.
            if key.startswith('list-'):
                if val.shape[0] == 1:
                    show_listelem_struct(key, val)
                else:
                    show_listelem_flat(key, val)
            elif key.startswith('waveform-'):
                show_waveformelem_struct(key, val)
            else:
                print "Root %s:" % root
                print "Node %s:  %s" % (key, val)
            # Keep following until we hit a leaf node
            show_entries(root[key])
        except:
            # just ignore leaf entries
            pass
    
if __name__ == '__main__':
    path = 'out.h5'
    if sys.argv[1:]:
        path = sys.argv[1]
    try:
        h5file = h5py.File(path, 'r', libver='latest', swmr=True)
    except Exception, exc:
        print "ERROR: failed to open %s in SWMR-mode: %s" % (path, exc)
        print "Are you sure it is a HDF5 file created/opened in SWMR mode?"
        sys.exit(1)
    root = h5file["/"]
    while True:
        print "Display contents"
        show_entries(root)
        print
        time.sleep(2)
