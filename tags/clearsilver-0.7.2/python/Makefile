

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../
endif

include $(NEOTONIC_ROOT)rules.mk

NEO_UTIL_SO = neo_cgi.so
NEO_UTIL_PYD = neo_cgi.pyd
NEO_UTIL_SRC = neo_cgi.c neo_util.c neo_cs.c
NEO_UTIL_OBJ = $(NEO_UTIL_SRC:%.c=%.o)



ifeq ($(PYTHON_INC),)
PYTHON_INC = -I/usr/include/python1.5
endif
PYTHON_INC += -I/usr/include/python1.5

CFLAGS += -I$(NEOTONIC_ROOT) $(PYTHON_INC)
DLIBS += -lneo_cgi -lneo_cs -lneo_utl 
LIBS += -L$(LIB_DIR) $(DLIBS) $(DB2_LIB)

ifeq ($(OSNAME),WindowsNT 0)
TARGETS = $(NEO_UTIL_PYD)
PYTHONLIBS=-LC:/python22/Libs -lpython22 
else
TARGETS = $(NEO_UTIL_SO)
endif

all: $(TARGETS)

$(NEO_UTIL_SO): $(NEO_UTIL_OBJ) $(DEP_LIBS)
	$(LDSHARED) -o $@ $(LDFLAGS) $(NEO_UTIL_OBJ) $(LIBS)

$(NEO_UTIL_PYD): $(NEO_UTIL_OBJ) $(DEP_LIBS)
	dllwrap --dllname neo_cgi.pyd --driver-name gcc \
		--def neo_cgi.def -o neo_cgi.pyd \
		$(NEO_UTIL_OBJ) -s --entry _DllMain@12 \
		--target=i386-mingw32 \
		$(PYTHONLIBS) $(LIBS)

clean:
	$(RM) *.o

distclean:
	$(RM) Makefile.depends $(TARGETS) *.o
