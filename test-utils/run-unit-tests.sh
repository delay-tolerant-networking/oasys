#!/bin/sh

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
    echo "*** $prog"
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
