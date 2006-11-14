

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ..
endif

include $(NEOTONIC_ROOT)/rules.mk

NEO_UTIL_SO = neo_cgi.so
NEO_UTIL_PYD = neo_cgi.pyd
NEO_UTIL_SRC = neo_cgi.c neo_util.c neo_cs.c
NEO_UTIL_OBJ = $(NEO_UTIL_SRC:%.c=%.o)

CFLAGS += $(PYTHON_INC)
DLIBS += -lneo_cgi -lneo_cs -lneo_utl 
LIBS += $(DLIBS) $(DB2_LIB)

ifeq ($(USE_MINGW32),1)
TARGETS = $(NEO_UTIL_PYD)
else
TARGETS = $(NEO_UTIL_SO)
endif

all: $(TARGETS)

$(NEO_UTIL_SO): setup.py $(NEO_UTIL_SRC) $(DEP_LIBS)
	rm -f $(NEO_UTIL_SO)
	CC="$(CC)" LDSHARED="$(LDSHARED)" $(PYTHON) setup.py build_ext --inplace

OLD_NEO_UTIL_SO:
	$(LDSHARED) -o $@ $(LDFLAGS) $(NEO_UTIL_OBJ) $(LIBS)

setup:
	$(PYTHON) setup.py build_ext --inplace

$(NEO_UTIL_PYD): $(NEO_UTIL_OBJ) $(DEP_LIBS)
	dllwrap --dllname neo_cgi.pyd --driver-name gcc \
		--def neo_cgi.def -o neo_cgi.pyd \
		$(NEO_UTIL_OBJ) -s --entry _DllMain@12 \
		--target=i386-mingw32 \
		$(LDFLAGS) $(PYTHON_LIB) $(LIBS)

install: all
	$(NEOTONIC_ROOT)/mkinstalldirs $(DESTDIR)$(PYTHON_SITE)
	$(INSTALL) $(TARGETS) $(DESTDIR)$(PYTHON_SITE)

clean:
	$(RM) *.o

distclean:
	$(RM) Makefile.depends $(TARGETS) *.o
	$(RM) -r build
