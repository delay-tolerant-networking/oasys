#!/usr/bin/python
import sys

class MemEntry:
    def __init__(self, live, stack):
	self.live  = live
	self.stack = stack
    def __str__(self):
	return "live = %d, stack = %s" % ( self.live, str(self.stack))

# read in memory usage dump
entries = []
f = open(sys.argv[1], 'r')
l = f.readline()
while l != "":
    l = l.strip().split(' ')
    entries.append(MemEntry(int(l[0]), l[1:]))
    l = f.readline()

# sort them by location
# XXX/bowei todo
