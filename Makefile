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
	compat/rpc_compat.c			\
	compat/xdr_int64_compat.c		\

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
	io/TTY.cc				\
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
	serialize/DebugSerialize.cc		\
	serialize/KeySerialize.cc		\
	serialize/MarshalSerialize.cc		\
	serialize/Serialize2Hash.cc		\
	serialize/SQLSerialize.cc		\
	serialize/Serialize.cc			\
	serialize/StreamSerialize.cc		\
	serialize/StringSerialize.cc		\
	serialize/TclListSerialize.cc		\
	serialize/TextSerialize.cc		\
	serialize/XMLSerialize.cc		\
	serialize/XercesXMLSerialize.cc		\
	serialize/StringPairSerialize.cc	\

SMTP_SRCS :=                                    \
	smtp/BasicSMTP.cc          		\
	smtp/SMTP.cc          			\
	smtp/SMTPClient.cc    			\
	smtp/SMTPServer.cc     			\
	smtp/SMTPUtils.cc     			\

STORAGE_SRCS :=					\
	storage/BerkeleyDBStore.cc		\
	storage/CheckedLog.cc			\
	storage/DurableStore.cc                 \
	storage/DurableStoreImpl.cc		\
	storage/FileBackedObject.cc		\
	storage/FileBackedObjectStore.cc	\
	storage/FileBackedObjectStream.cc	\
	storage/FileSystemStore.cc		\
	storage/MemoryStore.cc                  \
	storage/DS.cc				\
	storage/DataStore.cc			\
	storage/DataStoreProxy.cc		\
	storage/DataStoreServer.cc		\
	storage/ExternalDurableTableIterator.cc	\
	storage/ExternalDurableStore.cc		\
	storage/ExternalDurableTableImpl.cc	\

TCLCMD_SRCS :=					\
	tclcmd/ConsoleCommand.cc		\
	tclcmd/DebugCommand.cc			\
	tclcmd/GettimeofdayCommand.cc		\
	tclcmd/HelpCommand.cc			\
	tclcmd/IdleTclExit.cc			\
	tclcmd/LogCommand.cc			\
	tclcmd/TclCommand.cc			\
	tclcmd/tclreadline.c			\

THREAD_SRCS :=					\
	thread/Atomic-mutex.cc			\
	thread/LockDebugger.cc			\
	thread/Mutex.cc				\
	thread/NoLock.cc			\
	thread/Notifier.cc			\
	thread/OnOffNotifier.cc			\
	thread/SpinLock.cc			\
	thread/Thread.cc			\
	thread/Timer.cc				\

UTIL_SRCS :=					\
	util/App.cc				\
	util/Base16.cc				\
	util/CRC32.cc				\
	util/Daemonizer.cc			\
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
        util/URI.cc				\
	util/jenkins_hash.c			\
	util/jenkins_hash.cc			\
	util/md5-rsa.c				\

XML_SRCS :=					\
	xml/ExpatXMLParser.cc			\
	xml/XMLDocument.cc			\
	xml/XMLObject.cc			\

SRCS := \
	oasys-version.c				\
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

COMPAT_OBJS := $(COMPAT_SRCS:.c=.o) oasys-version.o

ALLSRCS := $(SRCS)

CPPS := $(SRCS:.cc=.E)
CPPS := $(CPPS:.c=.E)

TOOLS	:= \
	tools/md5chunks				\
	tools/oasys_tclsh			\
	tools/zsize				\
	test-utils/proc-watcher			\

all: checkconfigure prebuild libs $(TOOLS)

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
compat/xdr_int64_compat.o: compat/xdr_int64_compat.c
	$(CC) $(CPPFLAGS) $(CFLAGS_NOWARN) -c $< -o $@

debug/gdtoa-%.o: debug/gdtoa-%.c
	$(CC) -I./debug -DINFNAN_CHECK $(CPPFLAGS) $(CFLAGS_NOWARN) -c $< -o $@

#debug/_ldtoa.o: debug/_ldtoa.c
#	$(CC) $(CPPFLAGS) $(CFLAGS) -fno-strict-aliasing -c $< -o $@

debug/vfprintf.o: debug/vfprintf.c
	$(CC) $(CPPFLAGS) $(CFLAGS_NOWARN) -c $< -o $@

tclcmd/tclreadline.o: tclcmd/tclreadline.c
	$(CC) $(CPPFLAGS) $(CFLAGS_NOWARN) -c $< -o $@

#
# Include the Makefile for tests
#
include $(SRCDIR)/test/Makefile

#
# Include the common rules
#
include Rules.make

#
# Based on configuration options, select the libraries to build
#
LIBFILES := liboasys.a liboasyscompat.a
ifeq ($(SHLIBS),yes)
LIBFILES += liboasys.$(SHLIB_EXT) liboasyscompat.$(SHLIB_EXT)
LIBFILES += test-utils/libtclgettimeofday.$(SHLIB_EXT)
endif

.PHONY: libs
libs: $(LIBFILES)

#
# Need special rules for the gdtoa sources adapted from the source
# distribution.
#
debug/arith-native.h: debug/gdtoa-arithchk.c
	@mkdir -p debug
	$(CC) $(CPPFLAGS) $(CFLAGS_NOWARN) $< -o debug/arithchk
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
	$(CXX) -I./debug $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

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
# Rules for the version files
#
oasys-version.o: oasys-version.c
oasys-version.c: oasys-version.h
oasys-version.h: oasys-version.h.in oasys-version.dat
	tools/subst-version oasys-version.dat < 
	   $(SRCDIR)/oasys-version.h.in > oasys-version.h

vpath oasys-version.h.in $(SRCDIR)
vpath oasys-version.h    $(SRCDIR)
vpath oasys-version.c    $(SRCDIR)
vpath oasys-version.dat  $(SRCDIR)

bump-version:
	cd $(SRCDIR) && tools/bump-version oasys-version.dat

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

liboasys.a: $(OBJS)
	rm -f $@
	$(AR) ruc $@ $^
	$(RANLIB) $@ || true

liboasys.$(SHLIB_EXT): $(OBJS)
	rm -f $@
	$(CXX) $^ $(LDFLAGS_SHLIB) $(LDFLAGS) $(LIBS) $(OASYS_LIBS) -o $@

liboasyscompat.a: $(COMPAT_OBJS)
	rm -f $@
	$(AR) ruc $@ $^
	$(RANLIB) $@ || true

liboasyscompat.$(SHLIB_EXT): $(COMPAT_OBJS)
	rm -f $@
	$(CXX) $^ $(LDFLAGS_SHLIB) $(LDFLAGS) $(LIBS) -o $@

test-utils/libtclgettimeofday.$(SHLIB_EXT): test-utils/tclgettimeofday.c
	@rm -f $@; mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS_SHLIB) $(LDFLAGS) $(LIBS) $(OASYS_LIBS) -o $@


# Rules for linking tools
tools/md5chunks: tools/md5chunks.o $(LIBFILES)
	$(CXX) $(CFLAGS) $< -o $@ $(LDFLAGS) -L. -loasys $(LIBS)

tools/oasys_tclsh: tools/oasys_tclsh.o $(LIBFILES)
	$(CXX) $(CFLAGS) $< -o $@ $(LDFLAGS) -L. -loasys $(OASYS_LIBS) $(LIBS)

tools/zsize: tools/zsize.o $(LIBFILES)
	$(CXX) $(CFLAGS) $< -o $@ $(LDFLAGS) -L. -loasys $(LIBS)

test-utils/proc-watcher: test-utils/proc-watcher.o
	$(CXX) $(CFLAGS) $< -o $@ $(LDFLAGS)

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


# the --cxx-prologue and --cxx-epilogue add a guard to the generated c++ code
XSD_TOOL_ARGS := \
	--generate-serialization \
	--hxx-suffix .h \
	--cxx-suffix .cc \
	--root-element-all \
	--namespace-map =dsmessage \
	--cxx-prologue '\#include <config.h>' \
	--cxx-prologue '\#if defined(XERCES_C_ENABLED) && defined(EXTERNAL_DS_ENABLED)' \
	--cxx-epilogue '\#endif' \

xsdbindings: storage/DS.xsd
ifdef XSD_TOOL
	$(XSD_TOOL) cxx-tree $(XSD_TOOL_ARGS) --output-dir `dirname $<` $<
else
	@echo "WARNING: configure was unable to find the xsd tool needed to"
	@echo "         regenerate DS.h."
	@echo "         Use the --with-xsd-tool=(name) option with configure"
	@echo "         specify the location of this tool."
endif
