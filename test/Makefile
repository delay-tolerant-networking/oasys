# test/memory-test
TESTS += test/memory-test
ALLSRCS += test/memory-test.cc
test/memory-test: liboasys.a test/memory-test.o
	$(CPP) $^ -o $@ $(LFLAGS) $(LIBS)
