#!/bin/sh
#
# Simple script to run various autoconf related commands to generate
# the configure script. 
#
# This should not need to be run often, only when things like library
# dependencies change. In particular, all output files are cross-platform
# and are therefore checked into CVS.

trap 'rm -f aclocal.m4 ; exit 0' 0 1 2 3 13 15

echo "build-configure: building aclocal.m4..."
rm -f aclocal.m4
cat aclocal/*.ac > aclocal.m4

echo "build-configure: running autoscan to find missing checks..."
autoscan

echo "build-configure: running autoheader to build config.h.in..."
rm -f config.h config.h.in
autoheader
chmod 444 config.h.in

echo "build-configure: running autoconf to build configure..."
rm -f configure
autoconf
chmod 555 configure

echo "build-configure: purging configure cache..."
rm -rf autom4te.cache
rm -f config.cache

echo "build-configure: done."