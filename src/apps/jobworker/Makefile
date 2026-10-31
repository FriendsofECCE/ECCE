# Headless worker application. Keep this independent of wx so the same
# lifecycle executable can later run under a Windows pipe adapter or on the
# ECCE data server.
include $(ECCE_MAKEINC_DIR)/Makefile.defs
include $(ECCE_MAKEINC_DIR)/Makefile.xerces

SRCS   := $(shell ls *.C 2>/dev/null)
OBJS   = $(patsubst %.C, $(OBJ_PATH)/%.o, $(SRCS))
IFLAGS := $(ECCE_IFLAGS) $(XERCES_IFLAGS)
CFLAGS := $(ECCE_CFLAGS)
LD      := $(CXX)
LFLAGS  := $(ECCE_LFLAGS) $(XERCES_LFLAGS)
LDFLAGS = $(ECCE_LDFLAGS) $(LFLAGS)

ECCE_LIBS = \
            -leccecommxt \
            -leccercmd \
            -lecceedsi \
            -leccedav \
            -leccecipc \
            -leccefaces \
            -leccexml \
            -leccetdat \
            -lecceutil
LIBS = $(ECCE_LIBS) $(EXPECT_LIBS) $(XERCES_LIBS)
PROGRAMS = eccejobworker

default: $(OBJ_PATH) $(SRCS:%.C=.%.D) $(PROGRAMS)

include $(shell ls .*.D 2>/dev/null)
include $(ECCE_MAKEINC_DIR)/Makefile.rules
