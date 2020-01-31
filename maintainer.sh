#!/bin/sh
set -x

aclocal # Set up an m4 environment
autoconf # Generate configure from configure.ac
automake --add-missing # Generate Makefile.in from Makefile.am

#Test configure & build
./build.sh
#Cleanup
./cleanup.sh
