################################################################################
# FILENAME:  Makefile
#
#
# PURPOSE:
#   Makefile definition for ecce apps source tree.
#
# DESCRIPTION:
################################################################################

COMMONTARGETS = clean
SOURCEDIRS = $(shell ls | grep -v CVS | grep -v Makefile)

PARENTDIR = $(shell pwd)

apps: 
	@echo ${SOURCEDIRS}
	for dir in ${SOURCEDIRS} ;\
	do \
		if [ -d $${dir} ] ; then \
			cd $${dir} ; $(MAKE); cd ${PARENTDIR} ; \
		fi \
	done

fast: 
	@echo ${SOURCEDIRS}
	for dir in ${SOURCEDIRS} ;\
	do \
		if [ -d $${dir} ] ; then \
			cd $${dir} ; $(MAKE) -j 8; cd ${PARENTDIR} ; \
		fi \
	done

$(COMMONTARGETS):
	for dir in ${SOURCEDIRS} ;\
	do \
		if [ -d $${dir} ] ; then \
			cd $${dir} ; $(MAKE) $@ ; cd ${PARENTDIR} ; \
		fi \
	done

