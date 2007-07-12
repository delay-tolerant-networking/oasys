#!/bin/sh

#
#    Copyright 2005-2006 Intel Corporation
# 
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
# 
#        http://www.apache.org/licenses/LICENSE-2.0
# 
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

dry_run=0

echodo () {
    echo "> $*"
    if [ ! $dry_run = 1 ] ; then $* ; fi
}

build_name=""
build_dir="."
config_opts=""
build_tests=1

while [ $# != 0 ] ; do

if [ "$1" = "-n" ] ; then
    dry_run=1
    echo "> dry run"
    shift
fi

if [ "$1" = "--build_name" ] ; then
    build_name=$2
    shift; shift;
fi

if [ "$1" = "--build_dir" ] ; then
    build_dir=$2
    shift; shift;
fi

if [ "$1" = "--config_opts" ] ; then
    config_opts=$2
    shift; shift;
fi

if [ "$1" = "--path" ] ; then
    PATH=$2
    shift; shift;
fi

if [ "$1" = "--no_build_tests" ] ; then
    build_tests=0
    shift; shift;
fi

done

echo "***"
echo "*** Building $build_name"
echo "***"

srcdir="."
if [ $build_dir != "." ] ; then
    echodo rm -rf $build_dir
    echodo mkdir $build_dir
    echodo cd $build_dir
    srcdir=".."
fi

eval echodo $srcdir/configure -C $config_opts

MAKEFLAGS="-k"
if [ -f /proc/cpuinfo ] ; then
    nprocs=`cat /proc/cpuinfo | grep processor | wc -l`
else
    nprocs=1
fi

if [ $nprocs -gt 1 ]; then
    MAKEFLAGS="$MAKEFLAGS -j $nprocs"
fi

echodo make $MAKEFLAGS
if [ $build_tests = 1 ] ; then
    echodo make $MAKEFLAGS tests
fi

