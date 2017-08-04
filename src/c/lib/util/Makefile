################################################################################
# FILENAME:  Makefile
#
#
# PURPOSE:
#   Makefile definition for ecce util libraries source tree.
#
# DESCRIPTION:
################################################################################

COMMONTARGETS = clean
SOURCEDIRS = $(shell ls -d * | grep -v CVS)
PARENTDIR = $(shell pwd)
LIBRARY = libecceutil.a
SO_LIBRARY = $(patsubst %.a,%.so,$(LIBRARY))

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
	cd $(LIB_PATH)/shared; ar x ../libecceutil.a; \
	cd $(LIB_PATH); $(CXX) -shared $(ECCE_CFLAGS) -fPIC  -o libecceutil.so shared/*.o; \
	strip libecceutil.so
	rm -f $(LIB_PATH)/shared/*

