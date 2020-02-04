#!/bin/sh
set -x

aclocal # Set up an m4 environment
autoconf # Generate configure from configure.ac
automake --add-missing # Generate Makefile.in from Makefile.am

#Test configure & build
./configure --prefix=$PWD
make
make install

#Cleanup
#files created by auto* tools (m4 files, configure etc) will remain.
make clean
make uninstall
