#!/bin/sh
#
# Some Apple OS X installations of the gcc compiler contain the file
# /usr/include/gcc/darwin/<version>/c++/debug/debug.h. Since, by
# default, the filesystem is case-insensitive, this conflicts with the
# file debug/Debug.h from the oasys distribution.
#
# This script will rename the file debug.h to apple-debug.h, and will
# replace all #include references to reflect the new name. All
# original files are saved with the extension .orig.

trap 'echo $0: internal error -- must reconcile manually; exit 1' ERR 1 2 3 13 15

if test x$CXX = x ; then
    CXX=g++
fi

gccver=`$CXX --version | head -1`
gccver=`echo $gccver | sed 's/g++.*(GCC) //'`
gccver=`echo $gccver | sed 's/ .*//'`
gccver=`echo $gccver | sed 's/\([0-9]*\.[0-9]*\)\..*/\1/'`

if test x$gccver = x ; then
    echo "cannot determine the compiler version, bailing"
    exit 1
fi

if test ! -f /usr/include/gcc/darwin/$gccver/c++/debug/debug.h ; then
    echo "/usr/include/gcc/darwin/$gccver/c++/debug/debug.h does not exist, bailing"
    exit 1
fi

echo "$0: renaming debug/debug.h to debug/apple-debug.h..."
cd /usr/include/gcc/darwin/$gccver/c++
cp debug/debug.h debug/apple-debug.h
mv debug/debug.h debug/debug.h.orig

echo "$0: replacing #include <debug/debug.h> with #include <debug/apple-debug.h>"
echo "$0: in the following files..."

for f in `find . -type f -print` ; do
    grep '#include <debug/debug.h>' $f > /dev/null 2>&1 || continue

    echo "$0: $f..."
    mv $f $f.orig
    sed 's#<debug/debug.h>#<debug/apple-debug.h>#' < $f.orig > $f
done

echo "$0: done"
