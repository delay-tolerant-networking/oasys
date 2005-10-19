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
	debug/DebugUtils.cc			\
	debug/Formatter.cc			\
	debug/Log.cc				\
	debug/gdtoa-dmisc.c			\
	debug/gdtoa-dtoa.c			\
	debug/gdtoa-gdtoa.c			\
	debug/gdtoa-gmisc.c			\
	debug/gdtoa-misc.c			\

IO_SRCS :=					\
	io/BufferedIO.cc			\
	io/FdIOClient.cc			\
	io/FileIOClient.cc			\
	io/FileUtils.cc				\
	io/IO.cc				\
	io/IPClient.cc				\
	io/IPSocket.cc				\
	io/NetUtils.cc				\
	io/PrettyPrintBuffer.cc			\
	io/TCPClient.cc				\
	io/TCPServer.cc				\
	io/UDPClient.cc				\

MEMORY_SRCS :=                                  \
	memory/Memory.cc          

SERIALIZE_SRCS :=				\
	serialize/BufferedSerializeAction.cc    \
	serialize/MarshalSerialize.cc		\
	serialize/SQLSerialize.cc		\
	serialize/Serialize.cc			\
	serialize/StringSerialize.cc		\
	serialize/TextSerialize.cc		\

STORAGE_SRCS :=					\
	storage/BerkeleyDBStore.cc		\
	storage/DurableStore.cc                 \
	storage/DurableStoreImpl.cc		\

TCLCMD_SRCS :=					\
	tclcmd/ConsoleCommand.cc		\
	tclcmd/DebugCommand.cc			\
	tclcmd/HelpCommand.cc			\
	tclcmd/LogCommand.cc			\
	tclcmd/TclCommand.cc			\

THREAD_SRCS :=					\
	thread/Mutex.cc				\
	thread/Notifier.cc			\
	thread/SpinLock.cc			\
	thread/Thread.cc			\
	thread/Timer.cc				\

UTIL_SRCS :=					\
	util/CRC32.cc				\
	util/ExpandableBuffer.cc		\
	util/Getopt.cc				\
	util/HexDumpBuffer.cc			\
	util/InitSequencer.cc			\
	util/MD5.cc				\
	util/OptParser.cc			\
	util/Options.cc				\
	util/ProgressPrinter.cc			\
	util/Random.cc				\
	util/RateEstimator.cc			\
	util/StreamBuffer.cc			\
	util/StringBuffer.cc			\
	util/StringUtils.cc			\
	util/TextCode.cc			\
	util/URL.cc				\
	util/jenkins_hash.c			\
	util/jenkins_hash.cc			\
	util/md5-rsa.c				\

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
# Need special rules for the gdtoa sources adapted from the source
# distribution. These must be defined before the common rules
# are pulled in from Rules.make.
#
debug/arith-native.h: debug/gdtoa-arithchk.c
	mkdir -p debug
	$(CC) $(DEBUG) $(OPTIMIZE) $< -o debug/arithchk
	debug/arithchk > $@
	rm -f debug/arithchk

debug/arith.h:
	$(MAKE) debug/arith-$(TARGET).h
	cp debug/arith-$(TARGET).h $@

debug/Formatter.o:  debug/Formatter.cc debug/arith.h
	@rm -f $@; mkdir -p $(@D)
	$(CXX) $(CFLAGS) -I./debug -c $< -o $@

debug/gdtoa-%.o: debug/gdtoa-%.c debug/arith.h
	$(CC) -I./debug -g -DINFNAN_CHECK -c $< -o $@

GENFILES += debug/arith.h debug/arith-native.h debug/arithchk

#
# If srcdir isn't set by some other makefile, then it must be .
#
ifeq ($(SRCDIR),)
SRCDIR   := .
BUILDDIR := .
endif

#
# Include the common rules
#
include Rules.make

#
# And a special rule to build the command-init-tcl.c file from command.tcl
#
tclcmd/TclCommand.o: tclcmd/command-init-tcl.c
tclcmd/command-init-tcl.c: $(SRCDIR)/tclcmd/command-init.tcl
	rm -f $@
	echo "static const char* INIT_COMMAND = " > $@;
	cat $^ | sed 's|\\|\\\\|g' |\
		 sed 's|"|\\"|g' | \
		 sed 's|^|"|g' | \
		 sed "s|$$|\\\\n\"|g" >> $@;
	echo ";">> $@

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

Rules.make: $(SRCDIR)/Rules.make.in $(SRCDIR)/configure
	@[ ! x`echo "$(MAKECMDGOALS)" | grep clean` = x ] || \
	(echo "$@ is out of date, need to rerun configure" && \
	exit 1)

$(SRCDIR)/Rules.make.in:
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
# test files
#
include $(SRCDIR)/test/Makefile
TESTS := $(patsubst %,test/%,$(TESTS))
.PHONY: test tests
test tests: all $(TESTS)

# run tests
.PHONY: check
check:
	cd test; tclsh UnitTest.tcl

# tags
.PHONY: tag tags
tag tags:
	find . -name "*.cc" -o -name "*.h" | xargs ctags
	find . -name "*.cc" -o -name "*.h" | xargs etags
