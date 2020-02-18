#!/bin/sh
aclocal # Set up an m4 environment
autoconf # Generate configure from configure.ac
automake --add-missing # Generate Makefile.in from Makefile.am

#Test configure & build
./configure
make

#install sgxtop / sgxstat link
#
# -- uncomment if install is needed.
#sudo make install
#echo "+-----------------------------------------------------------------------------------------------------------------+"
#echo "|  sgxtop/sgxstat installed in /usr/local/bin/ (default) or at the path supplied in --prefix option of configure. |"
#echo "|  Check if the installed path is in your PATH environment variable.                                              |"
#echo "|  $ sudo make uninstall  - to remove installed binaries.                                                         |"
#echo "+-----------------------------------------------------------------------------------------------------------------+"

#uninstall will remove sgxtop/sgxstat from --prefix path (default: /usr/local/bin/)
#
# -- uncomment if uninstall is needed.
#sudo make uninstall

#Cleanup
#files created by auto* tools (m4 files, configure etc) will remain.
#
# -- uncomment if cleanup is needed.
#make clean
