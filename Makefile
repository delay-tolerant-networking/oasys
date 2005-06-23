#
# Makefile for oasys
#

#
# Source lists
#

COMPAT_SRCS :=					\
	compat/fpclassify.c			\
	compat/getopt_long.c			\
	compat/inet_aton.c			\

DEBUG_SRCS :=					\
	debug/gdtoa-dmisc.c			\
	debug/gdtoa-dtoa.c			\
	debug/gdtoa-gdtoa.c			\
	debug/gdtoa-gmisc.c			\
	debug/gdtoa-misc.c			\
	debug/DebugUtils.cc			\
	debug/Formatter.cc			\
	debug/Log.cc				\

IO_SRCS :=					\
	io/BufferedIO.cc			\
	io/IO.cc				\
	io/FdIOClient.cc			\
	io/FileIOClient.cc			\
	io/FileUtils.cc				\
	io/IPSocket.cc				\
	io/IPClient.cc				\
	io/NetUtils.cc				\
	io/PrettyPrintBuffer.cc			\
	io/TCPClient.cc				\
	io/TCPServer.cc				\
	io/UDPClient.cc				\

MEMORY_SRCS :=                                  \
	memory/Memory.cc          

SERIALIZE_SRCS :=				\
	serialize/Serialize.cc			\
	serialize/MarshalSerialize.cc		\
	serialize/SQLSerialize.cc		\

STORAGE_SRCS :=					\
	storage/DurableStore.cc			\
	storage/DurableStoreImpl.cc		\
	storage/BerkeleyDBStore.cc		\
	storage/StorageConfig.cc		\

TCLCMD_SRCS :=					\
	tclcmd/TclCommand.cc			\
	tclcmd/DebugCommand.cc			\
	tclcmd/HelpCommand.cc			\
	tclcmd/LogCommand.cc			\

THREAD_SRCS :=					\
	thread/Mutex.cc				\
	thread/Notifier.cc			\
	thread/SpinLock.cc			\
	thread/Thread.cc			\
	thread/Timer.cc				\

UTIL_SRCS :=					\
	util/CRC32.cc				\
	util/InitSequencer.cc			\
	util/jenkins_hash.c			\
	util/MD5.cc				\
	util/md5-rsa.c				\
	util/jenkins_hash.cc			\
	util/Getopt.cc				\
	util/HexDumpBuffer.cc			\
	util/Options.cc				\
	util/OptParser.cc			\
	util/ProgressPrinter.cc			\
	util/Random.cc				\
	util/RateEstimator.cc			\
	util/SDNV.cc				\
	util/ScratchBuffer.cc			\
	util/StreamBuffer.cc			\
	util/StringBuffer.cc			\
	util/StringUtils.cc			\
	util/URL.cc				\

SRCS := \
	version.c				\
	$(COMPAT_SRCS) 				\
	$(DEBUG_SRCS) 				\
	$(IO_SRCS) 				\
	$(MEMORY_SRCS)                          \
	$(SERIALIZE_SRCS)			\
	$(STORAGE_SRCS) 			\
	$(TCLCMD_SRCS)				\
	$(THREAD_SRCS)				\
	$(UTIL_SRCS)				\

OBJS := $(SRCS:.cc=.o)
OBJS := $(OBJS:.c=.o)

COMPAT_OBJS := $(COMPAT_SRCS:.c=.o) version.o

ALLSRCS := $(SRCS)

CPPS := $(SRCS:.cc=.E)
CPPS := $(CPPS:.c=.E)

#
# Default target is to build the library and the compat library
#
LIBFILES := liboasys.a liboasyscompat.a
all: checkconfigure $(LIBFILES)

#
# Rule to generate the doxygen documentation
#
doxygen:
	doxygen doc/doxyfile

#
# And a rule to make sure that configure has been run recently enough.
#
.PHONY: checkconfigure
checkconfigure: Rules.make

Rules.make: Rules.make.in configure
	@[ ! x`echo "$(MAKECMDGOALS)" | grep clean` = x ] || \
	(echo "$@ is out of date, need to rerun configure" && \
	exit 1)

Rules.make.in:
	@echo SRCDIR: $(SRCDIR)
	@echo error -- Makefile did not set SRCDIR properly
	@exit 1

# XXX/demmer handle .so as well
liboasys.a: $(OBJS)
	rm -f $@
	ar ruc $@ $^
	ranlib $@ || true

liboasyscompat.a: $(COMPAT_OBJS)
	rm -f $@
	ar ruc $@ $^
	ranlib $@ || true

.PHONY: cpps
cpps: $(CPPS)

#
# Need special rules for the gdtoa sources adapted from the source
# distribution.
#
debug/arith-native.h: debug/gdtoa-arithchk.c
	$(CC) $(DEBUG) $(OPTIMIZE) $< -o debug/arithchk
	debug/arithchk > $@
	rm -f debug/arithchk

debug/arith.h:
	$(MAKE) debug/arith-$(TARGET).h
	cp debug/arith-$(TARGET).h $@

debug/Formatter.cc: debug/arith.h

debug/gdtoa-%.o: debug/gdtoa-%.c debug/arith.h
	$(CC)  -g -DINFNAN_CHECK -c $< -o $@

GENFILES += debug/arith.h debug/arith-native.h debug/arithchk

#
# And a special rule to build the command-init-tcl.c file from command.tcl
#
tclcmd/TclCommand.o: tclcmd/command-init-tcl.c
tclcmd/command-init-tcl.c: tclcmd/command-init.tcl
	rm -f $@
	echo "static const char* INIT_COMMAND = " > $@;
	cat $^ | sed 's|\\|\\\\|g' |\
		 sed 's|"|\\"|g' | \
		 sed 's|^|"|g' | \
		 sed "s|$$|\\\\n\"|g" >> $@;
	echo ";">> $@

#
# test files
#
include test/Makefile
TESTS := $(patsubst %,test/%,$(TESTS))
.PHONY: test tests
test tests: $(TESTS)

# run tests
.PHONY: check
check:
	cd test; tclsh UnitTest.tcl

#
# Include the common rules
#
-include Rules.make
