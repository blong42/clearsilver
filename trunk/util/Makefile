

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../
endif

include $(NEOTONIC_ROOT)rules.mk

UTL_LIB = $(LIB_DIR)libneo_utl.a
UTL_SRC = neo_err.c neo_files.c neo_misc.c neo_test.c ulist.c neo_hdf.c \
	  neo_str.c neo_date.c wildmat.c neo_hash.c

ifeq ($(OSTYPE),WindowsNT)

CFLAGS += -Ios_win
UTL_SRC += snprintf.c os_win/regex.c os_win/mkstemp.c

else 

UTL_SRC += ulocks.c skiplist.c dict.c filter.c rcfs.c neo_net.c neo_server.c
CFLAGS += -DHAVE_GMTOFF
ifeq ($(USE_DB2),1)
UTL_SRC += wdb.c
endif

endif

# Eventually, this should be in a configure script somewhere...
# This version is portable enough (linux/freebsd) for now


UTL_OBJ = $(UTL_SRC:%.c=%.o)

TARGETS = $(UTL_LIB)

all: $(TARGETS)

$(UTL_LIB): $(UTL_OBJ)
	$(AR) $@ $(UTL_OBJ)

clean:
	$(RM) *.o

distclean:
	$(RM) Makefile.depends $(TARGETS) *.o
