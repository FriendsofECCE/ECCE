# genmake:  The name of the library this Makefile builds
LIBRARY := libeccewxviz.a

# genmake:  include all ECCE definitions
include $(ECCE_MAKEINC_DIR)/Makefile.defs

# genmake:  includes for packages, dependencies, rules
include $(ECCE_MAKEINC_DIR)/Makefile.xerces
include $(ECCE_MAKEINC_DIR)/Makefile.wx
include $(ECCE_MAKEINC_DIR)/Makefile.gl

# genmake:  ALL MODIFIABLE
SRCS   = $(shell ls *.C 2>/dev/null)
OBJS   = $(patsubst %.C, $(OBJ_PATH)/%.o, $(SRCS))
IFLAGS := $(ECCE_IFLAGS) $(XERCES_IFLAGS) $(WX_IFLAGS) $(GL_IFLAGS)
CFLAGS := $(ECCE_CFLAGS)

default: $(OBJ_PATH) $(SRCS:%.C=.%.D) $(LIB_PATH)/$(LIBRARY)

compile: $(OBJ_PATH) $(SRCS:%.C=.%.D) $(OBJS)

include $(shell ls .*.D)
include $(ECCE_MAKEINC_DIR)/Makefile.rules
