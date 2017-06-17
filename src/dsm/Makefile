################################################################################
# FILENAME:  Makefile
#
# RCS Information:
#   $Author: ecce_cm $                         $Locker$
#   $Revision: 22247 $                       $Date: 2012-02-22 15:29:37 -0800 (Wed, 22 Feb 2012) $
#
# PURPOSE:
#   Makefile definition for ecce comm libraries source tree.
#
# DESCRIPTION:
################################################################################

COMMONTARGETS = clean
SOURCEDIRS = $(shell ls -d * | grep -v CVS)
PARENTDIR = $(shell pwd)

LIBRARY1 = libeccedav.a
LIBRARY2 = libecceedsi.a
LIBRARY3 = libeccefaces.a
LIBRARY4 = libeccexml.a
LIBRARY5 = libeccecipc.a
SO_LIBRARY1 = $(patsubst %.a,%.so,$(LIBRARY1))
SO_LIBRARY2 = $(patsubst %.a,%.so,$(LIBRARY2))
SO_LIBRARY3 = $(patsubst %.a,%.so,$(LIBRARY3))
SO_LIBRARY4 = $(patsubst %.a,%.so,$(LIBRARY4))
SO_LIBRARY5 = $(patsubst %.a,%.so,$(LIBRARY5))

#Needed for paths etc
include $(ECCE_MAKEINC_DIR)/Makefile.defs

libs: 
	for dir in ${SOURCEDIRS} ;\
	do \
		if [ -d $${dir} ] ; then \
			cd $${dir} ; $(MAKE) ; cd ${PARENTDIR} ; \
		fi \
	done
ifeq ($(ECCE_SHARED_LIBS),true)
		$(MAKE) shared
endif

fast:
	for dir in ${SOURCEDIRS} ;\
	do \
		if [ -d $${dir} ] ; then \
			cd $${dir} ; $(MAKE) -j 8 compile ; cd ${PARENTDIR} ; \
		fi \
	done
	$(MAKE)

$(COMMONTARGETS):
	for dir in ${SOURCEDIRS} ;\
	do \
		if [ -d $${dir} ] ; then \
			cd $${dir} ; $(MAKE) $@ ; cd ${PARENTDIR} ; \
		fi \
	done

shared:
	if [ ! -d $(LIB_PATH)/shared ] ; then \
		mkdir $(LIB_PATH)/shared; \
	fi; \
	cd $(LIB_PATH)/shared; ar x ../$(LIBRARY1); \
	cd $(LIB_PATH); $(CXX) -shared $(ECCE_CFLAGS) -fPIC  -o $(SO_LIBRARY1) shared/*.o; \
	strip $(SO_LIBRARY1)
	rm -f $(LIB_PATH)/shared/*
	cd $(LIB_PATH)/shared; ar x ../$(LIBRARY2); \
	cd $(LIB_PATH); $(CXX) -shared $(ECCE_CFLAGS) -fPIC  -o $(SO_LIBRARY2) shared/*.o; \
	strip $(SO_LIBRARY2)
	rm -f $(LIB_PATH)/shared/*
	cd $(LIB_PATH)/shared; ar x ../$(LIBRARY3); \
	cd $(LIB_PATH); $(CXX) -shared $(ECCE_CFLAGS) -fPIC  -o $(SO_LIBRARY3) shared/*.o; \
	strip $(SO_LIBRARY3)
	rm -f $(LIB_PATH)/shared/*
	cd $(LIB_PATH)/shared; ar x ../$(LIBRARY4); \
	cd $(LIB_PATH); $(CXX) -shared $(ECCE_CFLAGS) -fPIC  -o $(SO_LIBRARY4) shared/*.o; \
	strip $(SO_LIBRARY4)
	rm -f $(LIB_PATH)/shared/*
	cd $(LIB_PATH)/shared; ar x ../$(LIBRARY5); \
	cd $(LIB_PATH); $(CXX) -shared $(ECCE_CFLAGS) -fPIC  -o $(SO_LIBRARY5) shared/*.o; \
	strip $(SO_LIBRARY5)
	rm -f $(LIB_PATH)/shared/*

