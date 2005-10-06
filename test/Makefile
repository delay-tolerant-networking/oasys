TESTS += \
	berkeley-db-test			\
	command-test				\
	durable-cache-test			\
	io-basic-test				\
	marshal-test				\
	optparser-test				\
	sample-test				\
	serialize-test				\
	spin-lock-test				\
	static-buffer-test			\
	string-hash-test			\
	timer-test				\
	type-collection-test			\

#
# Tests that need to be converted to the new framework
#
OLD_TESTS :=					\
	buffered-io-test			\
	hexdump-test				\
	io-intr-test                            \
	log-test				\
	log-reparse-test			\
	md5-test				\
	memory-test				\
	msg-queue-test				\
	mutex-test				\
	options-test				\
	tcp-socket-test				\
	tcp-timeout-read-test			\
	tcp-writeall-test			\
	thread-test				\
	url-test				\

#
# Make sure test executables are included in 'make clean'
#
BINFILES += $(TESTS)

#
# default rule for tests
#
test/%test: test/%test.cc liboasys.a
	@mkdir -p test
	$(CXX) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) liboasys.a

test/%Test: test/%Test.cc liboasys.a
	@mkdir -p test
	$(CXX) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) liboasys.a
