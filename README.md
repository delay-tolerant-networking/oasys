OASYS - Object-oriented Adaptors to SYStem interfaces
-----------------------------------------------------

OASYS is a C++ library that provides a set of wrapper classes and
utilities for systems programming projects.

External Requirements
---------------------

* *gcc/g++*	- The GNU Compiler Collection
* *pthreads / libc6* - GNU C Library: Shared libraries
* *make* - The GNU version of the "make" utility
* *tcl*	- The Tool Command Language

Optional External Packages
--------------------------

* *tclreadline* - GNU Readline Extension for Tcl/Tk
* *libreadline* - GNU readline and history libraries
* *tclx* - Extended Tcl - shared library
* *tcllib* - Standard Tcl Library
* *libdb* - Berkeley v4.2 - v4.7  Database Libraries [development]
* *libbluetooth* - BlueZ Linux Bluetooth library
* *libgoogle-perftools0* - Libraries for CPU and heap analysis
* *libxerces-cs-dev* - Validating XML parser library for C++
* *python* - Interactive high-level object-oriented language
* *xsd* - XML Data Binding for C++
* *zlib* - Compression library [runtime]

Installation
------------

```
./configure
make
make install
```

Contents
--------
```
oasys/compat/           portability and compatibility logic
oasys/debug/            debugging and logging support code
oasys/io/               I/O and networking support code
oasys/memory/           memory management and debugging support
oasys/serialize/        object serialization support
oasys/smtp/             SMTP (mail) protocol handlers
oasys/storage/          persistant storage interface
oasys/tclcmd/           tcl command infrastructure and support
oasys/test/             unit tests and other test files
oasys/thread/           thread / lock support implementation
oasys/util/             miscellaneous utility classes
```
