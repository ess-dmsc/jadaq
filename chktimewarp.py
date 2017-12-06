#!/usr/bin/python

"""A parser to inspect simple charge file dumps and locate any
unexpected time jumps between adjacent events."""

import os
import sys
import time

def usage(name):
    """Print usage help"""
    print "Usage: %s DUMP_PATH" % name
    print """parses DUMP_PATH on simple event dump format and ponits out any
unexpected jumps in time stamps."""
    
def parse_dump(dump_path):
    """Parses the simple dump file in path and return a list of entries"""
    try:
        dump_fd = open(dump_path, 'r')
        entries = []
        for line in dump_fd:
            entries.append(line.split())
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

   print "Parsing simple dump in %s" % dump_path
   entries = parse_dump(dump_path)
   expect_diff = -1
   warps = 0
   print "Checking %d parsed entries for time warps" % len(entries)
   if entries[1:]:
       for i in range(1, len(entries)):
           prev = int(entries[i-1][0])
           cur = int(entries[i][0])
           diff = cur - prev
           if expect_diff < 0:
               expect_diff = diff
           if abs(diff - expect_diff) > 10:
               warps +=1
               print "time warp of %d (%d vs %d) for entry %d" % \
                     (diff, cur, prev, i)
   print "Found %d time jumps much bigger than %d" % (warps, expect_diff)
           
