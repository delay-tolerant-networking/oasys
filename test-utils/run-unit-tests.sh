#!/bin/sh

(
found_tests=0
for i in *; do
    if [ -f $i -a -x $i ]; then
        found_tests=1
        echo "***"
	echo "*** $i"
	echo "***"
        ./$i
    fi
done

if [ $found_tests = 0 ]; then
    echo "warning: no tests found"
fi
) 2>&1

