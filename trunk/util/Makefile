

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../
endif

include $(NEOTONIC_ROOT)rules.mk

UTL_LIB = $(LIB_DIR)libneo_utl.a
UTL_SRC = neo_err.c neo_files.c neo_misc.c neo_test.c ulist.c neo_hdf.c \
	  neo_str.c neo_date.c wildmat.c 

ifneq ($(OSNAME),WindowsNT 0)
UTL_SRC += ulocks.c skiplist.c dict.c filter.c rcfs.c neo_net.c

ifeq ($(USE_DB2),1)
UTL_SRC += wdb.c
endif

else 
CFLAGS += -Ios_win
UTL_SRC += snprintf.c os_win/regex.c os_win/mkstemp.c

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
