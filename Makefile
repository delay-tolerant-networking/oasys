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
	io/IO.cc				\
	io/NetUtils.cc				\
	io/FdIOClient.cc			\
	io/FileIOClient.cc			\
	io/IPSocket.cc				\
	io/IPClient.cc				\
	io/TCPClient.cc				\
	io/TCPServer.cc				\
	io/UDPClient.cc				\
	io/UDPServer.cc				\

THREAD_SRCS :=					\
	thread/Mutex.cc				\
	thread/Notifier.cc			\
	thread/Thread.cc			\
	thread/Timer.cc				\

UTIL_SRCS :=					\
	util/RateEstimator.cc			\
	util/StringBuffer.cc			\
	util/URL.cc				\

SRCS := $(DEBUG_SRCS) 				\
	$(IO_SRCS) 				\
	$(THREAD_SRCS)				\
	$(UTIL_SRCS)				\

OBJS := $(SRCS:.cc=.o)
OBJS := $(OBJS:.c=.o)

ALLSRCS := $(SRCS)

#
# Default target is to build the library
#
all: liboasys

# XXX/demmer handle .so as well
liboasys: liboasys.a
liboasys.a: $(OBJS)
	rm -f $@
	ar ruc $@ $^
	ranlib $@ || true

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
	$(CC) -g -DINFNAN_CHECK -c $< -o $@

GENFILES += debug/arith.h debug/arith-native.h debug/arithchk

#
# Include the common rules
#
-include Rules.make
