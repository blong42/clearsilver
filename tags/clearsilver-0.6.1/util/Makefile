

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../
endif

include $(NEOTONIC_ROOT)rules.mk

UTL_LIB = $(LIB_DIR)libneo_utl.a
UTL_SRC = neo_err.c neo_files.c neo_misc.c neo_test.c ulist.c neo_hdf.c \
	  neo_str.c ulocks.c skiplist.c dict.c neo_date.c rcfs.c \
	  wildmat.c
ifeq ($(USE_DB2),1)
UTL_SRC += wdb.c
endif
UTL_OBJ = $(UTL_SRC:%.c=%.o)

TARGETS = $(UTL_LIB)

all: $(TARGETS)

$(UTL_LIB): $(UTL_OBJ)
	$(AR) $@ $(UTL_OBJ)

clean:
	$(RM) *.o

distclean:
	$(RM) Makefile.depend $(TARGETS) *.o
