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
PYTHON		= /usr/local/bin/python
PYTHON_BUILD_EXT= yes
CPPFLAGS_SYS  	= -DHAVE_CONFIG_H -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64  -Wno-long-double -I/usr/local/BerkeleyDB.4.5/include 
LIBS		= -lz -lreadline 
OASYS_LIBS	=  -ltcl8.4 -lxerces-c -ldb-4.5
LDFLAGS_SYS	= -L.  -L/usr/local/BerkeleyDB.4.5/lib   
LDFLAGS_SHLIB	= -dynamiclib -single_module -fPIC -DPIC
LFSDEFS         = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
TCL_LDFLAGS	=  -ltcl8.4
XSD_TOOL	= xsd
