

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../../
endif

include $(NEOTONIC_ROOT)rules.mk

HDFTEST_EXE = hdftest
HDFTEST_SRC = hdftest.c
HDFTEST_OBJ = $(HDFTEST_SRC:%.c=%.o)

LISTDIRTEST_EXE = listdir_test
LISTDIRTEST_SRC = listdir_test.c
LISTDIRTEST_OBJ = $(LISTDIRTEST_SRC:%.c=%.o)


CFLAGS += -I$(NEOTONIC_ROOT)/util
LIBS += -L$(LIB_DIR) -lneo_utl # -lefence

TARGETS = $(HDFTEST_EXE) $(LISTDIRTEST_EXE)

all: $(TARGETS)

$(HDFTEST_EXE): $(HDFTEST_OBJ) $(NTR_LIB)
	$(LD) $@ $(HDFTEST_OBJ) $(LIBS)

$(LISTDIRTEST_EXE): $(LISTDIRTEST_OBJ) $(NTR_LIB)
	$(LD) $@ $(LISTDIRTEST_OBJ) $(LIBS)

clean:
	$(RM) *.o

distclean:
	$(RM) $(TARGETS) *.o
