

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../
endif

include $(NEOTONIC_ROOT)rules.mk

NEO_UTIL_SO = neo_cgi.so
NEO_UTIL_SRC = neo_cgi.c neo_util.c neo_cs.c
NEO_UTIL_OBJ = $(NEO_UTIL_SRC:%.c=%.o)

ifeq ($(PYTHON_INC),)
PYTHON_INC = -I/usr/include/python1.5
endif
PYTHON_INC += -I/usr/include/python1.5

CFLAGS += -I$(NEOTONIC_ROOT) $(PYTHON_INC)
DLIBS += -lneo_cgi -lneo_cs -lneo_utl 
LIBS += -L$(LIB_DIR) $(DLIBS) $(DB2_LIB)

TARGETS = $(NEO_UTIL_SO)

all: $(TARGETS)

$(NEO_UTIL_SO): $(NEO_UTIL_OBJ) $(DEP_LIBS)
	$(LDSHARED) -o $@ $(LDFLAGS) $(NEO_UTIL_OBJ) $(LIBS)

clean:
	$(RM) *.o

distclean:
	$(RM) $(TARGETS) *.o
