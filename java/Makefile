
.SUFFIXES: .java .class

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ..
endif

include $(NEOTONIC_ROOT)/rules.mk

CLASSDIR = tmp
LIBDIR = lib

NEO_UTIL_SO = $(LIBDIR)/libclearsilver-jni.so
ifeq ($(OSTYPE),Darwin)
NEO_UTIL_SO = libclearsilver-jni.jnilib
endif
NEO_UTIL_JAVA_SRC = $(wildcard org/clearsilver/*.java org/clearsilver/jni/*.java)

NEO_UTIL_JAVA_JAR = $(LIBDIR)/clearsilver.jar
NEO_UTIL_SRC = j_neo_util.c j_neo_cs.c
NEO_UTIL_OBJ = $(NEO_UTIL_SRC:%.c=%.o)

CSTEST_SRC = org/clearsilver/test/CSTest.java
CSTEST_CLASS = $(CLASSDIR)/org/clearsilver/test/CSTest.class
CSTEST_DATA_DIR = org/clearsilver/test
CSTEST_GOLD = $(CSTEST_DATA_DIR)/testdata/CSTest.gold

CFLAGS += $(JAVA_INCLUDE_PATH)
DLIBS += -lneo_cgi -lneo_cs -lneo_utl -lstreamhtmlparser
LIBS += $(DLIBS)
ifneq ($(OSTYPE),Darwin)
LDFLAGS += -Wl,-soname=$(NEO_UTIL_SO)
endif

TARGETS = org_clearsilver_jni_JniHdf.h org_clearsilver_jni_JniCs.h $(NEO_UTIL_SO) $(NEO_UTIL_JAVA_JAR)

all: $(TARGETS) test

$(NEO_UTIL_JAVA_JAR): $(NEO_UTIL_JAVA_SRC)
	$(MKDIR) -p $(LIBDIR)
	$(RM) -r $(CLASSDIR)
	$(MKDIR) $(CLASSDIR)
	$(JAVAC) -d $(CLASSDIR) $(NEO_UTIL_JAVA_SRC)
	$(JAR) cf $(NEO_UTIL_JAVA_JAR) -C $(CLASSDIR) org
	$(RM) -r $(CLASSDIR)

org_clearsilver_jni_JniHdf.h: $(NEO_UTIL_JAVA_JAR)
	$(JAVAH) -classpath $(NEO_UTIL_JAVA_JAR) -jni org.clearsilver.jni.JniHdf

org_clearsilver_jni_JniCs.h: $(NEO_UTIL_JAVA_JAR)
	$(JAVAH) -classpath $(NEO_UTIL_JAVA_JAR) -jni org.clearsilver.jni.JniCs

$(CSTEST_CLASS): $(CSTEST_SRC)
	$(MKDIR) $(CLASSDIR)
	$(JAVAC) -classpath $(NEO_UTIL_JAVA_JAR) -d $(CLASSDIR) $(CSTEST_SRC)

makegold: $(CSTEST_CLASS)
	@/bin/sh -c "LD_LIBRARY_PATH=$(NEOTONIC_ROOT)/java/$(LIBDIR); export LD_LIBRARY_PATH; CLASSPATH=$(CLASSDIR):$(NEO_UTIL_JAVA_JAR); export CLASSPATH; $(JAVA_PATH)/bin/java org.clearsilver.test.CSTest $(CSTEST_DATA_DIR)" > javatest.gold
	@echo "Generated gold files"

copygold: $(CSTEST_GOLD)
	cp $(CSTEST_GOLD) javatest.gold

test: $(CSTEST_CLASS)
	@echo "Running java test"
	@failed=0; \
	rm -f javatest.out; \
	LD_LIBRARY_PATH=$(NEOTONIC_ROOT)/java/$(LIBDIR); export LD_LIBRARY_PATH; \
	CLASSPATH=$(CLASSDIR):$(NEO_UTIL_JAVA_JAR); export CLASSPATH; \
	$(JAVA_PATH)/bin/java org.clearsilver.test.CSTest $(CSTEST_DATA_DIR) > javatest.out; \
	diff javatest.out javatest.gold  > /dev/null 2>&1; \
	return_code=$$?; \
	if [ $$return_code -ne 0 ]; then \
	  diff javatest.out  javatest.gold > javatest.err; \
	  echo "Failed Java Test: CSTest"; \
	  echo "    See javatest.out and javatest.err"; \
	  failed=1; \
	fi; \
	if [ $$failed -eq 1 ]; then \
	  exit 1; \
	fi;
	@echo  "Passed java test"

$(NEO_UTIL_SO): $(NEO_UTIL_JAVA_JAR) $(NEO_UTIL_OBJ) $(DEP_LIBS)
	$(MKDIR) -p $(LIBDIR)
	$(LDSHARED) -o $@ $(LDFLAGS) $(NEO_UTIL_OBJ) $(LIBS)

# I guess we'll just stick the .jar file in the lib directory
install: all
	$(INSTALL) $(NEO_UTIL_SO) $(DESTDIR)$(libdir)
	$(INSTALL) $(NEO_UTIL_JAVA_JAR) $(DESTDIR)$(libdir)

clean:
	$(RM) *.o
	$(RM) -r $(CLASSDIR)
	$(RM) org_clearsilver_*.h
	$(RM) *.class
	$(RM) Makefile.depends

distclean: clean
	$(RM) -f Makefile.depends $(TARGETS)
