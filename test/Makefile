TESTS += \
	berkeley-db-test			\
	buffered-io-test			\
	command-test				\
	hexdump-test				\
	log-test				\
	log-reparse-test			\
	marshal-test				\
	md5-test				\
	memory-test				\
	msg-queue-test				\
	mutex-test				\
	options-test				\
	optparser-test				\
	sample-test				\
	serialize-test				\
	spin-lock-test				\
	static-buffer-test			\
	string-hash-test			\
	tcp-socket-test				\
	tcp-timeout-read-test			\
	tcp-writeall-test			\
	thread-test				\
	thread-interrupt-test			\
	timer-test				\
	url-test				\


#
# default rule for tests
#
test/%test: test/%test.cc liboasys.a
	$(CXX) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) liboasys.a
test/%Test: test/%Test.cc liboasys.a
	$(CXX) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) liboasys.a
