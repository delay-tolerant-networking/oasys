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



OASYS_TEST_UTILS_DIR=`echo $0 | sed 's|[^/]*$||'`
export OASYS_TEST_UTILS_DIR

expand_stacktrace=$OASYS_TEST_UTILS_DIR/expand-stacktrace.pl
if [ ! -x $expand_stacktrace ]; then
    echo "* WARNING: $expand_stacktrace not executable"
    expand_stacktrace=
fi

run_and_wait() {
    prog=$1

    echo "***"
    echo "*** Running Test: $prog"
    echo "***"

    if [ x$expand_stacktrace = x ] ; then
        ./$prog &
    else
        ./$prog 2>&1 | $expand_stacktrace -o $prog &
    fi
    pid=$!

    timeout=600
    while [ 1 ]; do
       PSOUT=`ps -p $pid 2> /dev/null | grep $pid`
       [ -z "$PSOUT" ] && break
       sleep 1

       timeout=$((timeout - 1))
       if [ $timeout == 0 ]; then
	   echo "error: unit test took too long -- killing it"
	   kill -ABRT $pid
	   sleep 5
	   continue
       fi
    done
}

(
found_tests=0
for i in *; do
    if [ -f $i -a -x $i ]; then
	if [ \( "$i" = "run" \) -o \( "$i" = "run-unit-tests.sh" \) ]; then
	    continue;
	fi

        found_tests=1
        run_and_wait $i 0
    fi
done

if [ $found_tests = 0 ]; then
    echo "warning: no tests found"
fi
) 2>&1
