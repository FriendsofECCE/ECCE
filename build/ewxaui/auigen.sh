#!/bin/bash

# This script generates the ECCE customized wxWidgets AUI source code from
# the standard wxAUI source code.  It must be run from a subdirectory of
# the bld directory under the top-level wxPython (which includes the
# wxWidgets distribution) source code distribution.  Otherwise the setting
# for WX_SRC will not be correct.  See the README file in this directory for
# instructions to build ewxAUI.

WX_SRC=../..

SED_SCRIPT=./auigen.sed

cp -f $WX_SRC/src/aui/auibar.cpp auibar.C
cp -f $WX_SRC/src/aui/auibook.cpp auibook.C
cp -f $WX_SRC/src/aui/dockart.cpp dockart.C
cp -f $WX_SRC/src/aui/floatpane.cpp floatpane.C
cp -f $WX_SRC/src/aui/framemanager.cpp framemanager.C
cp -f $WX_SRC/src/aui/tabmdi.cpp tabmdi.C

mkdir -p include/wx/ewxaui
cp -f $WX_SRC/include/wx/aui/auibar.h include/wx/ewxaui/auibar.H
cp -f $WX_SRC/include/wx/aui/auibook.h include/wx/ewxaui/auibook.H
cp -f $WX_SRC/include/wx/aui/dockart.h include/wx/ewxaui/dockart.H
cp -f $WX_SRC/include/wx/aui/floatpane.h include/wx/ewxaui/floatpane.H
cp -f $WX_SRC/include/wx/aui/framemanager.h include/wx/ewxaui/framemanager.H
cp -f $WX_SRC/include/wx/aui/tabmdi.h include/wx/ewxaui/tabmdi.H

sed -i -f $SED_SCRIPT auibar.C
sed -i -f $SED_SCRIPT auibook.C
sed -i -f $SED_SCRIPT dockart.C
sed -i -f $SED_SCRIPT floatpane.C
sed -i -f $SED_SCRIPT framemanager.C
sed -i -f $SED_SCRIPT tabmdi.C
sed -i -f $SED_SCRIPT include/wx/ewxaui/auibar.H
sed -i -f $SED_SCRIPT include/wx/ewxaui/auibook.H
sed -i -f $SED_SCRIPT include/wx/ewxaui/dockart.H
sed -i -f $SED_SCRIPT include/wx/ewxaui/floatpane.H
sed -i -f $SED_SCRIPT include/wx/ewxaui/framemanager.H
sed -i -f $SED_SCRIPT include/wx/ewxaui/tabmdi.H

./dockart_patch.pl
./floatpane_patch.pl
./framemanager_patch.pl

