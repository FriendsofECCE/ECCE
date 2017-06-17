################################################################################
# FILENAME:  Makefile
#
#
# PURPOSE:
#   Top level Makefile for ECCE.
#
# DESCRIPTION:
################################################################################

# include all ECCE definitions
include $(ECCE_MAKEINC_DIR)/Makefile.defs


default:
	$(MAKE) -C java/build
	$(MAKE) -C src
	$(MAKE) -C src/apps

fast:
	$(MAKE) -C java/build
	$(MAKE) -C src fast
	$(MAKE) -C src/apps fast

clean:
	$(MAKE) rmdep
	$(MAKE) rmobjs
	$(MAKE) rmlibs
	$(MAKE) rmbins
	$(MAKE) -C java/build clean

rmdep:
	@echo "removing all dependency files"
	@find . -name .\*.D -exec rm -f {} \; -print

rmobjs:
	@echo "removing all object files"
	rm -rf $(ECCE_HOME)/$(ECCE_SYSDIR)obj/*

rmlibs:
	@echo "removing all ECCE libraries"
	rm -rf $(LIB_PATH)/*

rmbins:
	@echo "removing all ECCE binaries"
	rm -rf $(BIN_PATH)/*

full:
# default target for templates is NOT update - so force
	$(MAKE) clean
	$(MAKE) default

fullfast:
	$(MAKE) clean
	$(MAKE) fast

