
.SUFFIXES: .java .class

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ..
endif

include $(NEOTONIC_ROOT)/rules.mk

NEO_UTIL_SO = libclearsilver-jni.so
ifeq ($(OSTYPE),Darwin)
NEO_UTIL_SO = libclearsilver-jni.jnilib
endif
NEO_UTIL_JAVA_SRC = HDF.java CS.java CSUtil.java CSFileLoader.java JNI.java # CGI.java

NEO_UTIL_JAVA_JAR = clearsilver.jar
NEO_UTIL_SRC = j_neo_util.c j_neo_cs.c
NEO_UTIL_OBJ = $(NEO_UTIL_SRC:%.c=%.o)

CFLAGS += $(JAVA_INCLUDE_PATH)
DLIBS += -lneo_cgi -lneo_cs -lneo_utl
LIBS += $(DLIBS)
ifneq ($(OSTYPE),Darwin)
LDFLAGS += -Wl,-soname=$(NEO_UTIL_SO)
endif

TARGETS = org_clearsilver_HDF.h org_clearsilver_CS.h $(NEO_UTIL_SO)

all: $(TARGETS) test

$(NEO_UTIL_JAVA_JAR): $(NEO_UTIL_JAVA_SRC)
	$(MKDIR) org/clearsilver
	$(JAVAC) -d . $(NEO_UTIL_JAVA_SRC)
	$(JAR) cf $(NEO_UTIL_JAVA_JAR) org
	$(RM) -r org

org_clearsilver_HDF.h: $(NEO_UTIL_JAVA_JAR)
	$(JAVAH) -classpath $(NEO_UTIL_JAVA_JAR) -jni org.clearsilver.HDF

org_clearsilver_CS.h: $(NEO_UTIL_JAVA_JAR)
	$(JAVAH) -classpath $(NEO_UTIL_JAVA_JAR) -jni org.clearsilver.CS

CSTest.class: CSTest.java
	$(JAVAC) -classpath $(NEO_UTIL_JAVA_JAR) CSTest.java

gold: CSTest.class
	@/bin/sh -c "LD_LIBRARY_PATH=$(NEOTONIC_ROOT)/java-jni; export LD_LIBRARY_PATH; CLASSPATH=$(NEO_UTIL_JAVA_JAR):.; export CLASSPATH; $(JAVA_PATH)/bin/java CSTest" > javatest.gold
	@echo "Generated gold files"

test: CSTest.class
	@echo "Running java test"
	@failed=0; \
	rm -f javatest.out; \
	LD_LIBRARY_PATH=$(NEOTONIC_ROOT)/java-jni; export LD_LIBRARY_PATH; \
	CLASSPATH=$(NEO_UTIL_JAVA_JAR):.; export CLASSPATH; \
	$(JAVA_PATH)/bin/java CSTest > javatest.out; \
	diff javatest.out javatest.gold  > /dev/null 2>&1; \
	return_code=$$?; \
	if [ $$return_code -ne 0 ]; then \
	  diff javatest.out javatest.gold > javatest.err; \
	  echo "Failed Java Test: CSTest"; \
	  echo "    See javatest.out and javatest.err"; \
	  failed=1; \
	fi; \
	if [ $$failed -eq 1 ]; then \
	  exit 1; \
	fi;
	@echo  "Passed java test"

CGI.h: CGI.class
	$(JAVAH) -jni CGI

$(NEO_UTIL_SO): $(NEO_UTIL_JAVA_JAR) $(NEO_UTIL_OBJ) $(DEP_LIBS)
	$(LDSHARED) -o $@ $(LDFLAGS) $(NEO_UTIL_OBJ) $(LIBS)

# I guess we'll just stick the .jar file in the lib directory
install: all
	$(INSTALL) $(NEO_UTIL_SO) $(DESTDIR)$(libdir)
	$(INSTALL) $(NEO_UTIL_JAVA_JAR) $(DESTDIR)$(libdir)

clean:
	$(RM) *.o *.so
	$(RM) -r org
	$(RM) org_clearsilver_*.h
	$(RM) *.class
	$(RM) org_clearsilver_HDF.h org_clearsilver_CS.h
	$(RM) Makefile.depends

distclean: clean
	$(RM) -f Makefile.depends $(TARGETS)
