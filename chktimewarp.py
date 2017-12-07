#!/usr/bin/python

"""A parser to inspect simple charge file dumps and locate any
unexpected time jumps between adjacent events."""

import os
import sys
import time

try:
    import numpy
    import matplotlib.pyplot as plt
    plot_enabled = True
except ImportError:
    print "Could not import matplotlib - no plot"
    plot_enabled = False

def usage(name):
    """Print usage help"""
    print "Usage: %s DUMP_PATH" % name
    print """parses DUMP_PATH on simple event dump format and ponits out any
unexpected jumps in time stamps."""
    
def parse_dump(dump_path, max_entries):
    """Parses the simple dump file in path and return a list of entries"""
    try:
        dump_fd = open(dump_path, 'r')
        entries = []
        count = 0
        for line in dump_fd:
            if max_entries > 0 and count > max_entries:
                break
            entries.append(line.split())
            count += 1
        dump_fd.close()
    except Exception, err:
        print "Error in parsing: %s" % err
    return entries

if __name__ == '__main__':
   if sys.argv[1:]:
       dump_path = sys.argv[1]
   else:
       usage(sys.argv[0])
       sys.exit(1)
   max_entries = -1
   if sys.argv[2:]:
       max_entries = int(sys.argv[2])

   print "Parsing simple dump in %s" % dump_path
   entries = parse_dump(dump_path, max_entries)
   expect_diff = None
   warps = 0
   if max_entries > 0:
       use_entries = min(len(entries), max_entries)
   else:
       use_entries = len(entries)
   print "Checking %d parsed entries for time warps" % len(entries)
   if entries[1:]:
       for i in range(1, use_entries):
           prev = int(entries[i-1][0])
           cur = int(entries[i][0])
           diff = cur - prev
           if expect_diff is None:
               expect_diff = diff
           # Warn if diff is bigger than expected, except after uint32 overflow
           if abs(diff - expect_diff) > 10 and cur > expect_diff:
               warps +=1
               print "time warp of %d (%d vs %d) for entry %d" % \
                     (diff, cur, prev, i)

       print "Found %d time jumps much bigger than %d" % (warps, expect_diff)

       if plot_enabled:
           events = numpy.arange(use_entries)
           times = numpy.array([int(entry[0]) for entry in entries[:use_entries]])
           plt.plot(events, times, label='Timestamps')
           plt.title('Event Timestamps')
           plt.xlabel('Event Number')
           plt.ylabel('Timestamp')
           plt.show()
   else:
       print "No entries to check for time jumps"

