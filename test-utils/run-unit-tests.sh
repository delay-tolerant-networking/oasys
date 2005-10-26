#!/bin/sh

run_and_wait() {
    prog=$1

    echo "***"
    echo "*** $prog"
    echo "***"

    $prog &
    pid=$!

    timeout=300
    while [ 1 ]; do
       ps -h $pid > /dev/null 2>&1
       [ $? = 1 ] && break
       sleep 1

       timeout=$((timeout - 1))
       if [ $timeout == 0 ]; then
	   echo "error: unit test took too long!!!"
	   kill -ABRT $pid
	   break
       fi
    done
}

(
found_tests=0
for i in *; do
    if [ -f $i -a -x $i ]; then
        found_tests=1
        run_and_wait $i 0
    fi
done

if [ $found_tests = 0 ]; then
    echo "warning: no tests found"
fi
) 2>&1

