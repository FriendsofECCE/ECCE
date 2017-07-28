# Perhaps this a good idea, but it likely isn't
# include $(ECCE_MAKEINC_DIR)/Makefile.defs

TOP_DIR ?= $(CURDIR)
ECCE_HOME ?= $(TOP_DIR)
# THIS SHOULD CHANGE TO build in the future, but that name is already taken
BUILD_DIR ?= $(TOP_DIR)/build_temp
LOG_DIR ?= $(BUILD_DIR)/ecce
THIRD_PARTY = $(BUILD_DIR)/3rdparty
THIRD_PARTY_TAR = $(TOP_DIR)/build/3rdparty-dists

# BUILD VARIABLES
ARCH ?= x86_64
BITS ?= 64
CC ?= gcc
CPP ?= g++
CFLAGS ?= ""
CPPFLAGS ?= ""

# Lets make the directory
$(BUILD_DIR) \
$(LOG_DIR)):
	@echo $@
	#mkdir -p $@

dir: $(BUILD_DIR) $(LOG_DIR)

all:
	@echo $(BUILD_DIR)

# Rules for building third party software
third_party: dir xerces

clean_third_party:
	rm -rf $(THIRD_PARTY)

######### XERCES #########
xerces: dir $(THIRD_PARTY)/src/xerces*/.built

# Untar the Xerces source
$(THIRD_PARTY)/src/xerces-c-src%:
	mkdir -p $(THIRD_PARTY)/src
	tar xvf $(THIRD_PARTY_TAR)/xerces-c-src* -C $(THIRD_PARTY)/src

# Build Xerces
# This process changes for xerces 3
# More info at https://xerces.apache.org/xerces-c/build-3.html
$(THIRD_PARTY)/src/xerces%/.built: $(THIRD_PARTY)/src/xerces%
	cd $</src/xercesc; ./runConfigure -p linux -b $(BITS) -P $(THIRD_PARTY)/install/xerces
	$(MAKE) -C $</src/xercesc
	$(MAKE) -C $</src/xercesc install
	@touch $</.built
