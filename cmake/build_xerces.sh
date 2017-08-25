#! /bin/bash

# $1 = XERCES_SRC
# $2 = XERCES_ROOT
# $3 = XERCES_INSTALL
# $4 = XERCES_PLATFORM
if [ ! -e $3/.installed ]
then
    cd $1
    export XERCESCROOT=$2
    ./runConfigure -p$4 -b64 -d -nfileonly -cgcc -xg++ -P$3
    make -C $1
    make -C $1 install
    touch $3/.installed
fi
