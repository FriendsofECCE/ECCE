# Perhaps this a good idea, but it likely isn't
# include $(ECCE_MAKEINC_DIR)/Makefile.defs

TOP_DIR ?= $(CURDIR)
ECCE_HOME ?= $(TOP_DIR)
# THIS SHOULD CHANGE TO build in the future, but that name is already taken
BUILD_DIR ?= $(TOP_DIR)/build_temp
LOG_DIR ?= $(BUILD_DIR)/log
THIRD_PARTY = $(BUILD_DIR)/3rdparty
THIRD_PARTY_TAR = $(TOP_DIR)/build/3rdparty-dists

# BUILD VARIABLES
ARCH ?= x86_64
BITS ?= 64
# CC ?= gcc
# CPP ?= g++
# CFLAGS ?= ""
# CPPFLAGS ?= ""

# Lets make the directory
$(BUILD_DIR) \
$(LOG_DIR) \
$(THIRD_PARTY):
	@echo $@
	mkdir -p $@

dir: $(BUILD_DIR) $(LOG_DIR) $(THIRD_PARTY)

all:
	@echo $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

######### THIRD PARTY PACKAGES #########
# Rules for building third party software
third_party: dir

clean_third_party:
	rm -rf $(THIRD_PARTY)

######### XERCES #########
xerces: $(THIRD_PARTY)/src/xerces*/.built

# Build Xerces
# This process changes for xerces 3
# More info at https://xerces.apache.org/xerces-c/build-3.html
#$(THIRD_PARTY)/src/xerces%/.built: $(THIRD_PARTY)/src/xerces%
#	@echo $<
#	cd $</src/xercesc; ./runConfigure -p linux -b $(BITS) -P $(THIRD_PARTY)/install/xerces
#	$(MAKE) -C $</src/xercesc
#	$(MAKE) -C $</src/xercesc install
#	@touch $</.built

%.build:
	@echo This is specific

# Untar the Xerces source
$(THIRD_PARTY)/src/%: dir
	@echo $(@F)
	mkdir -p $(THIRD_PARTY)/src
	# @tar xvf $(THIRD_PARTY_TAR)/xerces* -C $(THIRD_PARTY)/src
