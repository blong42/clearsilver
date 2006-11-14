

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ..
endif

include $(NEOTONIC_ROOT)/rules.mk

NEO_SO = libneo.so
NEO_STATIC_LIBS =  

LIBS += $(DLIBS) $(DB2_LIB)


TARGETS = $(NEO_SO) dsotest

all: $(TARGETS)

$(NEO_SO): $(NEO_STATIC_LIBS) $(DEP_LIBS) Makefile 
	 /usr/bin/ld  -rpath ../libs -shared -o libneo.so -whole-archive ../libs/libneo_cgi.a ../libs/libneo_cs.a ../libs/libneo_utl.a -no-whole-archive $(LDFLAGS) $(LIBS)

#	$(LDSHARED) -o $@ $(LDFLAGS) -Wl,-whole-archive $(DLIBS)

install: all
	$(NEOTONIC_ROOT)/mkinstalldirs $(DESTDIR)$(PYTHON_SITE)
	$(INSTALL) $(TARGETS) $(DESTDIR)$(PYTHON_SITE)

dsotest: dsotest.c $(NEO_SO)
	gcc -o dsotest dsotest.c -lneo -L. -I..

clean:
	$(RM) *.o 

distclean:
	$(RM) Makefile.depends $(TARGETS) *.o
