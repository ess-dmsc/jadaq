#!/usr/bin/python

"""Simple hdf5 dumper from python, which reads HDF5 files in SWMR mode"""

import os
import sys
import time
import h5py

def show_entries(root):
    """Print out the contents of root and recursively apply to nested items"""
    for (key, val) in root.items():
        print "Root %s:" % root
        print "%s = %s" % (key, val)
        try:
            show_entries(root[key])
        except:
            # just ignore leaf entries
            pass
    
if __name__ == '__main__':
    path = 'out.h5'
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
