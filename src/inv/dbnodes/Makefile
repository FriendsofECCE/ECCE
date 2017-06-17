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
include $(ECCE_MAKEINC_DIR)/Makefile.gl

# genmake:  ALL MODIFIABLE
SRCS1  = $(shell ls *.C 2>/dev/null)
SRCS2  = $(shell ls *.c 2>/dev/null)
SRCS   = $(SRCS1) $(SRCS2)
OBJS1  = $(patsubst %.C, $(OBJ_PATH)/%.o, $(SRCS1))
OBJS2  = $(patsubst %.c, $(OBJ_PATH)/%.o, $(SRCS2))
OBJS   = $(OBJS1) $(OBJS2)
IFLAGS := $(ECCE_IFLAGS) $(GL_IFLAGS)
CFLAGS := $(ECCE_CFLAGS) -DNDEBUG
CCFLAGS := $(ECCE_CCFLAGS) -DNDEBUG
 
default: $(OBJ_PATH) $(SRCS1:%.C=.%.D) $(SRCS2:%.c=.%.D) $(LIB_PATH)/$(LIBRARY)

compile: $(OBJ_PATH) $(SRCS1:%.C=.%.D) $(SRCS2:%.c=.%.D) $(OBJS1) $(OBJS2)

include $(shell ls .*.D)
include $(ECCE_MAKEINC_DIR)/Makefile.rules
