TESTS += \
	buffered-io-test	\
	command-test		\
	hexdump-test		\
	log-test 		\
	log-reparse-test	\
	marshal-test 		\
	md5-test 		\
	memory-test 		\
	msg-queue-test 		\
	mutex-test 		\
	options-test 		\
	serialize-test          \
	spin-lock-test          \
	tcp-socket-test 	\
	tcp-timeout-read-test 	\
	tcp-writeall-test 	\
	thread-test 		\
	timer-test 		\
	url-test 		\

#
# default rule for tests
#
test/%-test: test/%-test.cc liboasys.a
	$(CPP) $^ -o $@ $(CFLAGS) $(LIBS) liboasys.a
