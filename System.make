#
# System.make: settings extracted from the oasys configure script that
# are common to the local system installation
#
# System.make.  Generated from System.make.in by configure.

AR		= ar
RANLIB		= ranlib
DEPFLAGS	= -MMD -MP -MT "$*.o $*.E $*.po"
INSTALL 	= /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA 	= ${INSTALL} -m 644
PYTHON		= /usr/bin/python
PYTHON_BUILD_EXT= yes
CPPFLAGS_SYS  	= -DHAVE_CONFIG_H -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64  
LIBS		= -lz -lreadline -ldl -lm -lpthread 
OASYS_LIBS	=  -ltcl8.4 -lexpat -lxerces-c -ldb-4.4
LDFLAGS_SYS	= -L.   -Wl,-rpath=. -Wl,-rpath=$(prefix)/lib 
LDFLAGS_SHLIB	= -shared -fPIC -DPIC
LFSDEFS         = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
TCL_LDFLAGS	=  -ltcl8.4
XSD_TOOL	= 
