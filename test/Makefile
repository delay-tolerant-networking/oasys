TESTS += \
	atomic-test				\
	berkeley-db-test			\
	buffer-test				\
	durable-cache-test			\
	filesys-db-test				\
	io-basic-test				\
	log-test				\
	marshal-test				\
	memory-store-test			\
	msg-queue-test				\
	open-fd-cache-test                      \
	optparser-test				\
	regex-test				\
	sample-test				\
	serialize-stream-test			\
	serialize-test				\
	smtp-test				\
	sparse-bitmap-test			\
	spin-lock-test				\
	stack-trace-test			\
	static-buffer-test			\
	string-hash-test			\
	string-tokenize-test			\
	text-code-test				\
	timer-test				\
	token-bucket-test			\
	type-collection-test		        \
	util-test				\

#
# Tests that need to be fixed
#
BROKEN_TESTS +=  \
	ptr-cache-test				\

#
# Tests that need to be converted to the new framework
#
OLD_TESTS :=					\
	buffered-io-test			\
	hexdump-test				\
	md5-test				\
	memory-test				\
	mutex-test				\
	options-test				\
	tcp-socket-test				\
	tcp-timeout-read-test			\
	tcp-writeall-test			\
	thread-test				\
	url-test				\

TEST_SRCS := $(patsubst %,test/%.cc,$(TESTS))
ALLSRCS += $(TEST_SRCS)

#
# Make sure test executables are included in 'make clean'
#
BINFILES += $(TESTS)

#
# default rule for tests
#
test/%test: test/%test.cc liboasys.a
	@mkdir -p test
	$(CXX) $< -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) liboasys.a $(OASYS_LIBS)

test/%Test: test/%Test.cc liboasys.a
	@mkdir -p test
	$(CXX) $< -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) liboasys.a $(OASYS_LIBS)

#
# Override rules for stack-trace-test to disable the cast-align warnings
#
test/stack-trace-test: test/stack-trace-test.cc liboasys.a
	@mkdir -p test
	$(CXX) $< -o $@ $(CFLAGS) -Wno-cast-align \
	   $(LDFLAGS) $(LIBS) liboasys.a $(OASYS_LIBS)

#
# special rules for the smtp support scripts used in cases where
# the build directory is not the source directory
#
TESTFILES += \
	smtp-test-send.py \
	smtp-test-send.tcl

test/smtp-test-send.py test/smtp-test-send.tcl:
	rm -f $@
	ln -s $(SRCDIR)/$@ $@
