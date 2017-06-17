
# Makefile wrapper around ant so jar is automatically built as needed
# either when it is missing or a .java file is newer

JAR = $(ECCE_HOME)/java/lib/ecce_jms.jar
SRCS = $(shell ls $(ECCE_HOME)/java/src/gov/pnnl/emsl/ecce/jms/*.java 2>/dev/null)

default: $(JAR)

$(JAR): $(SRCS)
	ant

clean:
	ant clean

