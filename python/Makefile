

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../
endif

include $(NEOTONIC_ROOT)rules.mk

NEO_UTIL_SO = neo_util.so
NEO_UTIL_SRC = neo_util.c
NEO_UTIL_OBJ = $(NEO_UTIL_SRC:%.c=%.o)

PYTHON_INC = /usr/include/python1.5

CFLAGS += -I$(NEOTONIC_ROOT) -I$(HOME)/src/db-2.7.7/dist -I$(PYTHON_INC)
DLIBS += -lneo_cgi -lneo_cs -lneo_utl 
LIBS += -L$(LIB_DIR) $(DLIBS) -lz -L$(HOME)/src/db-2.7.7/dist -ldb

TARGETS = $(NEO_UTIL_SO)

all: $(TARGETS)

$(NEO_UTIL_SO): $(NEO_UTIL_OBJ) $(DEP_LIBS)
	$(LDSHARED) -o $@ $(LDFLAGS) $(NEO_UTIL_OBJ) $(LIBS)

clean:
	$(RM) *.o

distclean:
	$(RM) $(TARGETS) *.o
