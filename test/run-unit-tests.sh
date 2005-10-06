#!/bin/sh

for i in *; do
    if [ \( -d $i \) -o \( $i = "UnitTest.tcl" \) -o \( $i = "run-unit-tests.sh" \) ]; then
	continue
    fi

    if [ -x $i ]; then
	is_unit_test=`grep "UnitTest\\.h" $i.cc`
	if [ ! -z "$is_unit_test" ]; then
	    echo "***"
	    echo "*** $i"
	    echo "***"
	    ./$i
	fi
    fi
done
