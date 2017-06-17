################################################################################
# FILENAME:  Makefile
#
# DESCRIPTION:
#
# CUSTOMIZATIONS:
################################################################################

# genmake:  The name of the library this Makefile builds
LIBRARY := libecceinv.a
 
# genmake:  include all ECCE definitions
include $(ECCE_MAKEINC_DIR)/Makefile.defs
 
# genmake:  includes for packages, dependencies, rules

# genmake:  ALL MODIFIABLE
SRCS   = $(shell ls *.c 2>/dev/null)
OBJS   = $(patsubst %.c, $(OBJ_PATH)/%.o, $(SRCS))
IFLAGS := $(ECCE_IFLAGS)
CCFLAGS := $(ECCE_CCFLAGS)
 
default: $(OBJ_PATH) $(SRCS:%.c=.%.D) $(LIB_PATH)/$(LIBRARY)

compile: $(OBJ_PATH) $(SRCS:%.c=.%.D) $(OBJS)

include $(shell ls .*.D)
include $(ECCE_MAKEINC_DIR)/Makefile.rules
