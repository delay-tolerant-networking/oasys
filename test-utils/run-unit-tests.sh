#!/bin/sh

for i in *; do
    if [ -f $i -a -x $i ]; then
        echo "***"
	echo "*** $i"
	echo "***"
        ./$i
    fi
done
