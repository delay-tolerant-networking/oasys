#
# Makefile for oasys
#

#
# Source lists
#

DEBUG_SRCS :=					\
	debug/gdtoa-dmisc.c			\
	debug/gdtoa-dtoa.c			\
	debug/gdtoa-gdtoa.c			\
	debug/gdtoa-gmisc.c			\
	debug/gdtoa-misc.c			\
	debug/Formatter.cc			\
	debug/Log.cc				\

IO_SRCS :=					\
	io/BufferedIO.cc			\
	io/IO.cc				\
	io/FdIOClient.cc			\
	io/FileIOClient.cc			\
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

TCLCMD_SRCS :=					\
	tclcmd/TclCommand.cc			\
	tclcmd/HelpCommand.cc			\
	tclcmd/LogCommand.cc			\

THREAD_SRCS :=					\
	thread/Mutex.cc				\
	thread/Notifier.cc			\
	thread/SpinLock.cc			\
	thread/Thread.cc			\
	thread/Timer.cc				\

UTIL_SRCS :=					\
	util/jenkins_hash.c			\
	util/md5-rsa.c				\
	util/jenkins_hash.cc			\
	util/Options.cc				\
	util/RateEstimator.cc			\
	util/StreamBuffer.cc			\
	util/StringBuffer.cc			\
	util/URL.cc				\

SRCS := $(DEBUG_SRCS) 				\
	$(IO_SRCS) 				\
	$(MEMORY_SRCS)                          \
	$(SERIALIZE_SRCS)			\
	$(TCLCMD_SRCS)				\
	$(THREAD_SRCS)				\
	$(UTIL_SRCS)				\

OBJS := $(SRCS:.cc=.o)
OBJS := $(OBJS:.c=.o)

ALLSRCS := $(SRCS)

#
# Default target is to build the library
#
all: checkconfigure liboasys

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

Rules.make.in:
	@echo SRCDIR: $(SRCDIR)
	@echo error -- Makefile did not set SRCDIR properly
	@exit 1

Rules.make: Rules.make.in configure
	@echo $@ is out of date, need to rerun configure
	@exit 1

# XXX/demmer handle .so as well
LIBFILES += liboasys.a
liboasys: liboasys.a
liboasys.a: $(OBJS)
	rm -f $@
	ar ruc $@ $^
	ranlib $@ || true

#
# Need special rules for the gdtoa sources adapted from the source
# distribution.
#
debug/arith.h: debug/gdtoa-arithchk.c
	$(CC) $(DEBUG) $(OPTIMIZE) $< -o debug/arithchk
	debug/arithchk > $@
	rm -f debug/arithchk

debug/Formatter.cc: debug/arith.h

debug/gdtoa-%.o: debug/gdtoa-%.c debug/arith.h
	$(CC) -g -DINFNAN_CHECK -c $< -o $@

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
# Include test files
#
include test/Makefile
.PHONY: tests
tests: $(TESTS)

#
# Include the common rules
#
-include Rules.make
