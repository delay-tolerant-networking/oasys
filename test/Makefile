# test/memory-test
TESTS += test/memory-test
ALLSRCS += test/memory-test.cc
test/memory-test: test/memory-test.o liboasys.a
	$(CPP) $^ -o $@ $(LFLAGS) $(LIBS)

TESTS += test/timer-test
ALLSRCS += test/timer-test.cc
test/timer-test: test/timer-test.o liboasys.a
	$(CPP) $^ -o $@ $(LFLAGS) $(LIBS)

TESTS += test/hexdump-test
ALLSRCS += test/hexdump-test.cc
test/hexdump-test: test/hexdump-test.o liboasys.a
	$(CPP) $^ -o $@ $(LFLAGS) $(LIBS)
