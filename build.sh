#!/bin/sh

set -x
./configure --prefix=$PWD
make
make install
