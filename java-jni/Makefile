
.SUFFIXES: .java .class

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../
endif

include $(NEOTONIC_ROOT)rules.mk

NEO_UTIL_SO = libclearsilver-jni.so
NEO_UTIL_JAVA_SRC = HDF.java CS.java # CGI.java
NEO_UTIL_JAVA_CLASS = $(NEO_UTIL_JAVA_SRC:%.java=%)
NEO_UTIL_JAVA_CLASSFILES = $(NEO_UTIL_JAVA_SRC:%.java=%.class)
NEO_UTIL_JAVA_JAR = clearsilver.jar
NEO_UTIL_SRC = j_neo_util.c j_neo_cs.c
NEO_UTIL_OBJ = $(NEO_UTIL_SRC:%.c=%.o)


CWD = /home/jeske/neotonic/java-jni

ifeq ($(JAVA_INC),)
JAVA_INC = -I/usr/java/j2sdk1.4.1/include/ -I/usr/java/j2sdk1.4.1/include/linux
endif

CFLAGS += -I$(NEOTONIC_ROOT) $(JAVA_INC)
DLIBS += -lneo_cgi -lneo_cs -lneo_utl 
LIBS += -L$(LIB_DIR) $(DLIBS) -lz $(DB2_LIB)

TARGETS = $(NEO_UTIL_SO)

all: $(TARGETS) test

$(NEO_UTIL_JAVA_JAR): $(NEO_UTIL_JAVA_SRC)
	javac $(NEO_UTIL_JAVA_SRC)
	mkdir -p org/clearsilver
	cp $(NEO_UTIL_JAVA_CLASSFILES) org/clearsilver
	jar cf $(NEO_UTIL_JAVA_JAR) org
	rm -rf org

org_clearsilver_HDF.h: $(NEO_UTIL_JAVA_JAR)
	javah -classpath $(NEO_UTIL_JAVA_JAR) -jni org.clearsilver.HDF

org_clearsilver_CS.h: $(NEO_UTIL_JAVA_JAR)
	javah -classpath $(NEO_UTIL_JAVA_JAR) -jni org.clearsilver.CS

CSTest.class: CSTest.java
	javac -classpath $(NEO_UTIL_JAVA_JAR) CSTest.java

test: CSTest.class
	/bin/sh -c "LD_LIBRARY_PATH=$(CWD); export LD_LIBRARY_PATH; CLASSPATH=$(NEO_UTIL_JAVA_JAR):.; export CLASSPATH; java CSTest"
CGI.h: CGI.class
	javah -jni CGI

$(NEO_UTIL_SO): $(NEO_UTIL_JAVA_JAR) $(NEO_UTIL_OBJ) $(DEP_LIBS)
	$(LDSHARED) -o $@ $(LDFLAGS) $(NEO_UTIL_OBJ) $(LIBS)

clean:
	$(RM) *.o *.so
	rm -rf org
	rm -f org_clearsilver_*.h
	rm -f *.class
	rm -f org_clearsilver_HDF.h org_clearsilver_CS.h
	rm -f Makefile.depends

distclean: clean
	$(RM) -f $(TARGETS)
