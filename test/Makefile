# test/memory-test
TESTS += test/memory-test
ALLSRCS += test/memory-test.cc
test/memory-test: test/memory-test.o liboasys.a
	$(CPP) $^ -o $@ $(LFLAGS) $(LIBS)
