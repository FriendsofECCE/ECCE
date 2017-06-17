# Makefile for ewxAUI, an enhanced version of the standard wxWidgets wxAUI.
# This Makefile must be invoked from a subdirectory of the bld directory
# under the top-level wxPython (which includes the wxWidgets distribution)
# source code distribution. Otherwise the WX_BUILD_DIR setting will not be
# correct. See the README file in this directory for instructions to build
# ewxAUI.
#
# This Makefile was derived from one created for the wxcode repository of
# extensions to wxWidgets.  The original copyright information is given below.
# ----------------------------------------------------------------------------
# Author : John Labenski
# Copyright : John Labenski 2006
# License : wxWidgets license
# ----------------------------------------------------------------------------


# ----------------------------------------------------------------------------
# wxWidgets build settings

WX_BUILD_DIR := $(shell ./grepBuildDir)

WXCONFIG   := $(WX_BUILD_DIR)/bin/wx-config
PKGCONFIG  := pkg-config

WXBASENAME := $(shell $(WXCONFIG) --basename)
WXVERSION  := $(shell $(WXCONFIG) --version)
WXRELEASE  := $(shell $(WXCONFIG) --release)

WXCXX      := $(shell $(WXCONFIG) --cxx)
WXCXXFLAGS := $(shell $(WXCONFIG) --cxxflags)
WXLIBS     := $(shell $(WXCONFIG) --libs)

WXDIR      := $(shell $(WXCONFIG) --prefix)
WXEXECDIR  := $(shell $(WXCONFIG) --exec-prefix)
WXLIBDIR   := $(WXEXECDIR)/lib
WXINCDIR   := $(WXEXECDIR)/include/wx-$(WXRELEASE)

# ----------------------------------------------------------------------------
# Settings for the library

EWXAUI_LIBDIR  = $(WXLIBDIR)
EWXAUI_LIBNAME = $(WXBASENAME)_ewxaui-$(WXRELEASE)
EWXAUI_INCDIR = $(WXINCDIR)/wx/ewxaui

LIBVERSION_CURRENT  = 1
LIBVERSION_REVISION = 0
LIBVERSION_AGE      = 0

TARGET_LIBNAME = lib$(EWXAUI_LIBNAME)
TARGET_LIBNAME_STATIC = $(TARGET_LIBNAME).a
TARGET_LIBNAME_SHARED = $(TARGET_LIBNAME).so.$(LIBVERSION_CURRENT).$(LIBVERSION_REVISION).$(LIBVERSION_AGE)
TARGET_LIB_STATIC = $(EWXAUI_LIBDIR)/$(TARGET_LIBNAME_STATIC)
TARGET_LIB_SHARED = $(EWXAUI_LIBDIR)/$(TARGET_LIBNAME_SHARED)
TARGET_LIB_LINK1  = $(TARGET_LIBNAME).so.$(LIBVERSION_CURRENT)
TARGET_LIB_LINK2  = $(TARGET_LIBNAME).so
TARGET_INC = \
        $(EWXAUI_INCDIR)/auibar.H \
        $(EWXAUI_INCDIR)/auibook.H \
        $(EWXAUI_INCDIR)/dockart.H \
        $(EWXAUI_INCDIR)/floatpane.H \
        $(EWXAUI_INCDIR)/framemanager.H \
        $(EWXAUI_INCDIR)/tabmdi.H

# ----------------------------------------------------------------------------
# find the GTK headers

ifndef GTK_CFLAGS
    ifeq (gtk2, $(findstring gtk2, $(WXBASENAME)))
        GTK_CFLAGS := $(shell $(PKGCONFIG) --cflags gtk+-2.0)
    else
        ifeq (gtk, $(findstring gtk, $(WXBASENAME)))
            GTK_CFLAGS := $(shell gtk-config --cflags)
        else
            GTK_CFLAGS =
        endif
    endif
endif

# ----------------------------------------------------------------------------
# CXX flags and libs

PROFILE = #-pg
CXXFLAGS = $(WXCXXFLAGS) $(GTK_CFLAGS) -MMD -g -Wall -fPIC -DPIC -I./include $(PROFILE)
LDLIBS   = $(WXLIBS)
CXX      = $(WXCXX)

# ----------------------------------------------------------------------------
# Programs to use

RM        = rm -f
AR        = ar
AROPTIONS = rcu
RANLIB    = ranlib
LN_S      = ln -s

# ----------------------------------------------------------------------------
# Source files, objects to link

OBJECTS = \
	auibar.o \
	auibook.o \
	dockart.o \
	floatpane.o \
	framemanager.o \
	tabmdi.o

HEADERS = \
	include/wx/ewxaui/auibar.H \
	include/wx/ewxaui/auibook.H \
	include/wx/ewxaui/dockart.H \
	include/wx/ewxaui/floatpane.H \
	include/wx/ewxaui/framemanager.H \
	include/wx/ewxaui/tabmdi.H

DEPFILES := $(OBJECTS:.o=.d)

.cpp.o:
	$(CXX) -c $(CXXFLAGS) -o $@ $<

all: $(TARGET_LIB_STATIC) $(TARGET_LIB_SHARED) $(TARGET_INC)

$(TARGET_INC) : $(HEADERS)
	rm -rf $(EWXAUI_INCDIR)
	mkdir $(EWXAUI_INCDIR)
	cp $(HEADERS) $(EWXAUI_INCDIR) 

$(TARGET_LIB_STATIC) : $(OBJECTS)
	@$(RM) $(TARGET_LIB_STATIC)
	$(AR) $(AROPTIONS) $(TARGET_LIB_STATIC) $(OBJECTS)
	$(RANLIB) $(TARGET_LIB_STATIC)

$(TARGET_LIB_SHARED) : $(OBJECTS)
	$(CXX) -shared -o $(TARGET_LIB_SHARED) $(OBJECTS) $(WXLIBS)
	cd $(EWXAUI_LIBDIR) \
	&& $(RM) $(TARGET_LIB_LINK1) $(TARGET_LIB_LINK2) \
	&& $(LN_S) $(TARGET_LIBNAME_SHARED) $(TARGET_LIB_LINK1) \
	&& $(LN_S) $(TARGET_LIBNAME_SHARED) $(TARGET_LIB_LINK2)

clean:
	rm -f $(OBJECTS) $(DEPFILES) core \
		$(TARGET_LIB_STATIC) \
		$(TARGET_LIB_SHARED) \
		$(EWXAUI_LIBDIR)/$(TARGET_LIB_LINK1) \
		$(EWXAUI_LIBDIR)/$(TARGET_LIB_LINK2) \
        && rm -rf $(EWXAUI_INCDIR)


-include $(DEPFILES)
