#
#    Copyright 2004-2006 Intel Corporation
# 
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
# 
#        http://www.apache.org/licenses/LICENSE-2.0
# 
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

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
	compat/editline_compat.c		\

DEBUG_SRCS :=					\
	debug/DebugUtils.cc			\
	debug/DebugDumpBuf.cc			\
	debug/FatalSignals.cc			\
	debug/Formatter.cc			\
	debug/Log.cc				\
	debug/StackTrace.cc			\
	debug/gdtoa-dmisc.c			\
	debug/gdtoa-dtoa.c			\
	debug/gdtoa-gdtoa.c			\
	debug/gdtoa-gmisc.c			\
	debug/gdtoa-misc.c			\
	debug/_hdtoa.c				\
	debug/_ldtoa.c				\
	debug/vfprintf.c			\

IO_SRCS :=					\
	io/BufferedIO.cc			\
	io/FdIOClient.cc			\
	io/FileIOClient.cc			\
	io/FileUtils.cc				\
	io/IO.cc				\
	io/IPClient.cc				\
	io/IPSocket.cc				\
	io/MmapFile.cc				\
	io/NetUtils.cc				\
	io/PrettyPrintBuffer.cc			\
	io/RateLimitedSocket.cc			\
	io/TCPClient.cc				\
	io/TCPServer.cc				\
	io/UDPClient.cc				\

BLUEZ_SRCS :=					\
	bluez/Bluetooth.cc			\
	bluez/BluetoothSDP.cc			\
	bluez/BluetoothSocket.cc		\
	bluez/BluetoothInquiry.cc		\
	bluez/BluetoothClient.cc		\
	bluez/BluetoothServer.cc		\
	bluez/RFCOMMClient.cc			\

MEMORY_SRCS :=                                  \
	memory/Memory.cc          

SERIALIZE_SRCS :=				\
	serialize/BufferedSerializeAction.cc	\
	serialize/KeySerialize.cc		\
	serialize/MarshalSerialize.cc		\
	serialize/SQLSerialize.cc		\
	serialize/Serialize.cc			\
	serialize/StringSerialize.cc		\
	serialize/TclListSerialize.cc		\
	serialize/TextSerialize.cc		\
	serialize/XMLSerialize.cc		\
	serialize/XercesXMLSerialize.cc		\

SMTP_SRCS :=                                    \
	smtp/BasicSMTP.cc          		\
	smtp/SMTP.cc          			\
	smtp/SMTPClient.cc    			\
	smtp/SMTPServer.cc     			\
	smtp/SMTPUtils.cc     			\

STORAGE_SRCS :=					\
	storage/BerkeleyDBStore.cc		\
	storage/DurableStore.cc                 \
	storage/DurableStoreImpl.cc		\
	storage/FileSystemStore.cc		\
	storage/MemoryStore.cc                  \

TCLCMD_SRCS :=					\
	tclcmd/ConsoleCommand.cc		\
	tclcmd/DebugCommand.cc			\
	tclcmd/HelpCommand.cc			\
	tclcmd/LogCommand.cc			\
	tclcmd/TclCommand.cc			\
	tclcmd/tclreadline.c			\

THREAD_SRCS :=					\
	thread/Atomic-mutex.cc			\
	thread/Mutex.cc				\
	thread/NoLock.cc			\
	thread/Notifier.cc			\
	thread/OnOffNotifier.cc			\
	thread/SpinLock.cc			\
	thread/Thread.cc			\
	thread/Timer.cc				\

UTIL_SRCS :=					\
	util/Base16.cc				\
	util/CRC32.cc				\
	util/ExpandableBuffer.cc		\
	util/Getopt.cc				\
	util/Glob.cc				\
	util/HexDumpBuffer.cc			\
	util/InitSequencer.cc			\
	util/MD5.cc				\
	util/OptParser.cc			\
	util/Options.cc				\
	util/ProgressPrinter.cc			\
	util/Random.cc				\
	util/RateEstimator.cc			\
	util/RefCountedObject.cc		\
	util/Regex.cc				\
	util/Singleton.cc			\
	util/StreamBuffer.cc			\
	util/StringAppender.cc			\
	util/StringBuffer.cc			\
	util/StringUtils.cc			\
	util/TextCode.cc			\
	util/Time.cc                            \
	util/TokenBucket.cc                     \
	util/URL.cc				\
	util/jenkins_hash.c			\
	util/jenkins_hash.cc			\
	util/md5-rsa.c				\

XML_SRCS :=					\
	xml/ExpatXMLParser.cc			\
	xml/XMLDocument.cc			\
	xml/XMLObject.cc			\

SRCS := \
	version.c				\
	$(COMPAT_SRCS) 				\
	$(DEBUG_SRCS) 				\
	$(IO_SRCS) 				\
	$(BLUEZ_SRCS) 				\
	$(MEMORY_SRCS)                          \
	$(SERIALIZE_SRCS)			\
	$(SMTP_SRCS)				\
	$(STORAGE_SRCS) 			\
	$(TCLCMD_SRCS)				\
	$(THREAD_SRCS)				\
	$(UTIL_SRCS)				\
	$(XML_SRCS)				\

OBJS := $(SRCS:.cc=.o)
OBJS := $(OBJS:.c=.o)

COMPAT_OBJS := $(COMPAT_SRCS:.c=.o) version.o

ALLSRCS := $(SRCS)

CPPS := $(SRCS:.cc=.E)
CPPS := $(CPPS:.c=.E)

TOOLS	:= \
	tools/md5chunks				\
	tools/zsize				\

#
# Default target is to build the library, the compat library, and the tools
#
LIBFILES := liboasys.a liboasyscompat.a
all: checkconfigure prebuild $(LIBFILES) $(TOOLS)

# need to generate files first
.PHONY: prebuild
prebuild: debug/arith.h

#
# If srcdir/builddir aren't set by some other makefile, 
# then default to .
#
ifeq ($(SRCDIR),)
SRCDIR	 := .
BUILDDIR := .
endif

#
# Special override rules for objects that can't use the default build options
#
debug/gdtoa-%.o: debug/gdtoa-%.c
	$(CC) -I./debug -g -DINFNAN_CHECK -c $< -o $@

debug/vfprintf.o: debug/vfprintf.c
	$(CC) $(CPPFLAGS) $(DEBUG) $(OPTIMIZE) $(PROFILE) -c $< -o $@

tclcmd/tclreadline.o: tclcmd/tclreadline.c
	$(CC) $(CPPFLAGS) $(DEBUG) $(OPTIMIZE) $(PROFILE) -c $< -o $@

#
# Include the Makefile for tests
#
include $(SRCDIR)/test/Makefile

#
# Include the common rules
#
include Rules.make

#
# Need special rules for the gdtoa sources adapted from the source
# distribution.
#
debug/arith-native.h: debug/gdtoa-arithchk.c
	@mkdir -p debug
	$(CC) $(DEBUG) $(OPTIMIZE) $< -o debug/arithchk
	debug/arithchk > $@
	rm -f debug/arithchk

#
# If this is a native build (the default), then we build
# arith-native.h by running arithchk and copy it into arith.h
#
debug/arith.h: 
ifeq ($(TARGET),native)
	@mkdir -p debug
	$(MAKE) debug/arith-native.h
	@echo "copying debug/arith-native.h -> debug/arith.h"
	cp debug/arith-native.h debug/arith.h

#
# For a cross-compile build, look for debug/arith-$(TARGET).h. If
# that's not there, then try chopping everything after a '-' from
# $(TARGET) and trying again. This way, a target of 'arm-linux' will
# still find debug/arith-arm.h.
#
else
	@mkdir -p debug
	@t=$(TARGET); t2=`echo $(TARGET) | sed 's/-.*//'`;  \
        if test -f $(SRCDIR)/debug/arith-$$t.h ; then \
	    echo "copying $(SRCDIR)/debug/arith-$$t.h -> debug/arith.h" ; \
            cp $(SRCDIR)/debug/arith-$$t.h debug/arith.h; \
        elif test -f $(SRCDIR)/debug/arith-$$t2.h ; then \
	    echo "copying $(SRCDIR)/debug/arith-$$t2.h -> debug/arith.h" ; \
            cp $(SRCDIR)/debug/arith-$$t2.h debug/arith.h; \
	else \
	    echo "ERROR: can't find appropriate arith.h for cross-compiled target" ; \
	    echo "(tried debug/arith-$$t.h and debug/arith-$$t2.h)" ; \
	    echo "" ; \
	    echo "Try compiling and running debug/arithchk on the target machine" ; \
	    echo "and put the result in debug/arith-$$t.h". ; \
	    exit 1 ; \
	fi
endif

debug/Formatter.o:  debug/Formatter.cc debug/arith.h
	@rm -f $@; mkdir -p $(@D)
	$(CXX) $(CFLAGS) -I./debug -c $< -o $@

GENFILES += debug/arith.h debug/arith-native.h debug/arithchk

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
# Generate XML schema documentation
# Requires:
#     xsddoc-1.0 (http://xframe.sourceforge.net/xsddoc/)
#     java
#
xsddoc:
	mkdir -p doc/router-xsddoc
	xsddoc -t "External Router Interface" -o doc/router-xsddoc -q \
		daemon/router.xsd

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
	$(AR) ruc $@ $^
	$(RANLIB) $@ || true

liboasyscompat.a: $(COMPAT_OBJS)
	rm -f $@
	$(AR) ruc $@ $^
	$(RANLIB) $@ || true

# Rules for linking tools
tools/md5chunks: tools/md5chunks.o liboasys.a
	$(CXX) $(CFLAGS) $< -o $@ $(LDFLAGS) -L. -loasys $(LIBS)

tools/zsize: tools/zsize.o liboasys.a
	$(CXX) $(CFLAGS) $< -o $@ $(LDFLAGS) -L. -loasys $(LIBS)

.PHONY: cpps
cpps: $(CPPS)

# build tests
.PHONY: test tests testfiles
TESTS := $(patsubst %,test/%,$(TESTS))
TESTFILES := $(patsubst %,test/%,$(TESTFILES))
test tests: all $(TESTS) $(TESTFILES)

# run tests
.PHONY: check
check:
	cd test; tclsh UnitTest.tcl

# tags
.PHONY: tag tags
tag tags:
	find . -name "*.cc" -o -name "*.h" | xargs ctags
	find . -name "*.cc" -o -name "*.h" | xargs etags
