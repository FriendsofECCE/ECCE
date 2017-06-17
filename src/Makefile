################################################################################
# FILENAME:  Makefile
#
#
# PURPOSE:
#   Makefile definition for ecce source library tree.
#
# DESCRIPTION:
################################################################################

COMMONTARGETS = fast clean shared
SOURCEDIRS = util tdat dsm wxgui comm inv viz wxviz
PARENTDIR = $(shell pwd)

libs:
	for dir in ${SOURCEDIRS} ;\
	do \
		if [ -d $${dir} ] ; then \
           		cd $${dir} ; $(MAKE) ; cd ${PARENTDIR};\
		fi \
	done

$(COMMONTARGETS):
	for dir in ${SOURCEDIRS} ;\
	do \
		if [ -d $${dir} ] ; then \
			cd $${dir} ; $(MAKE) $@ ; cd ${PARENTDIR};\
		fi \
	done

