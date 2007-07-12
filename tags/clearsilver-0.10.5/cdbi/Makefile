
#
# needs C-DBI package to compile. We used 0.6.7 but perhaps
# newer ones would work...
#
# http://libdbi.sourceforge.net/

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../
endif

include $(NEOTONIC_ROOT)rules.mk

DEBUG=0

ifeq ($(DEBUG),1)
EFENCE=-lefence
endif

CDBI_LIB = $(LIB_DIR)libcdbi.a
CDBI_SRC = cdbi.c
CDBI_OBJ = $(CDBI_SRC:%.c=%.o)

ABTEST_EXE = abtest
ABTEST_SRC = cdbi_ab.c abtest.c
ABTEST_OBJ = $(ABTEST_SRC:%.c=%.o)

CFLAGS += -I$(NEOTONIC_ROOT) -I$(HOME)/src/db-2.7.7/dist `glib-config --cflags`
DLIBS += -lcdbi -lneo_cgi -lneo_cs -lneo_rtv -lneo_utl
LIBS += -L$(LIB_DIR) $(DLIBS) $(EFENCE) -ldbi -ldl -lz -L$(HOME)/src/db-2.7.7/dist -ldb `glib-config --libs`

TARGETS = $(CDBI_LIB) $(ABTEST_EXE)

all: $(TARGETS)

$(CDBI_LIB): $(CDBI_OBJ)
	$(AR) $@ $(CDBI_OBJ)

cdbi_ab.h: ab_db.py
	./cdbi_gen.py ab ab_db.py DB	

cdbi_ab.c: ab_db.py cdbi_ab.h

$(ABTEST_EXE): $(ABTEST_OBJ) $(DEP_LIBS) $(CDBI_LIB)
	$(LD) $@ $(ABTEST_OBJ) $(LIBS)

clean:
	$(RM) *.o

distclean:
	$(RM) Makefile.depends $(TARGETS) *.o
