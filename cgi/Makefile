

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../
endif

include $(NEOTONIC_ROOT)rules.mk

NTR_LIB = $(LIB_DIR)libneo_rtv.a
NTR_SRC = lexicon.c extent.c
NTR_OBJ = $(NTR_SRC:%.c=%.o)

LEXTEST_EXE = lextest
LEXTEST_SRC = lextest.c
LEXTEST_OBJ = $(LEXTEST_SRC:%.c=%.o)

EXTTEST_EXE = ext_test
EXTTEST_SRC = ext_test.c
EXTTEST_OBJ = $(EXTTEST_SRC:%.c=%.o)

CFLAGS += -I../util
LIBS += -L$(LIB_DIR) -lneo_rtv -lneo_utl # -lefence

TARGETS = $(NTR_LIB) $(LEXTEST_EXE) $(EXTTEST_EXE)

all: $(TARGETS)

$(NTR_LIB): $(NTR_OBJ)
	$(AR) $@ $(NTR_OBJ)

$(LEXTEST_EXE): $(LEXTEST_OBJ) $(NTR_LIB)
	$(LD) $@ $(LEXTEST_OBJ) $(LIBS)

$(EXTTEST_EXE): $(EXTTEST_OBJ) $(NTR_LIB)
	$(LD) $@ $(EXTTEST_OBJ) $(LIBS)

clean:
	$(RM) *.o

distclean:
	$(RM) $(TARGETS) *.o
