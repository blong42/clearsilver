

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../
endif

include $(NEOTONIC_ROOT)rules.mk

CS_LIB = $(LIB_DIR)libneo_cs.a
CS_SRC = csparse.c
CS_OBJ = $(CS_SRC:%.c=%.o)

CSTEST_EXE = cstest
CSTEST_SRC = cstest.c
CSTEST_OBJ = $(CSTEST_SRC:%.c=%.o)

CSDUMP_EXE = csdump
CSDUMP_SRC = csdump.c
CSDUMP_OBJ = $(CSDUMP_SRC:%.c=%.o)

CFLAGS += -I$(NEOTONIC_ROOT)
LIBS += -L$(LIB_DIR) -lneo_cs -lneo_utl

TARGETS = $(CS_LIB) $(CSTEST_EXE) $(CSDUMP_EXE)

all: $(TARGETS)

$(CS_LIB): $(CS_OBJ)
	$(AR) $@ $(CS_OBJ)

$(CSTEST_EXE): $(CSTEST_OBJ) $(CS_LIB)
	$(LD) $@ $(CSTEST_OBJ) $(LIBS)

$(CSDUMP_EXE): $(CSDUMP_OBJ) $(CS_LIB)
	$(LD) $@ $(CSDUMP_OBJ) $(LIBS)

clean:
	$(RM) *.o

distclean:
	$(RM) $(TARGETS) *.o
