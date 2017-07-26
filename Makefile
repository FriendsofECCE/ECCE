# Perhaps this a good idea, but it likely isn't
# include $(ECCE_MAKEINC_DIR)/Makefile.defs

TOP_DIR ?= `pwd`
# THIS SHOULD CHANGE TO build in the future, but that name is already taken
BUILD_DIR ?= $(TOP_DIR)/build_temp
THIRD_PARTY = $(BUILD_DIR)/3rdparty

all:
	@echo $(BUILD_DIR)

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
