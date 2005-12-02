

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ..
endif

include $(NEOTONIC_ROOT)/rules.mk

UTL_LIB = $(LIB_DIR)libneo_utl.a
UTL_SRC = neo_err.c neo_files.c neo_misc.c neo_rand.c ulist.c neo_hdf.c \
	  neo_str.c neo_date.c wildmat.c neo_hash.c $(EXTRA_UTL_SRC)

UTL_OBJ = $(UTL_SRC:%.c=%.o) $(EXTRA_UTL_OBJS)
UTL_HDR = $(UTL_SRC:%.c=%.h)

TARGETS = $(UTL_LIB)

all: $(TARGETS)

$(UTL_LIB): $(UTL_OBJ)
	$(AR) $@ $(UTL_OBJ)
	$(RANLIB) $@

install: all
	$(NEOTONIC_ROOT)/mkinstalldirs $(DESTDIR)$(cs_includedir)/util
	$(INSTALL) -m 644 $(UTL_HDR) $(DESTDIR)$(cs_includedir)/util
	$(INSTALL) -m 644 $(UTL_LIB) $(DESTDIR)$(libdir)

clean:
	$(RM) *.o

distclean:
	$(RM) Makefile.depends $(TARGETS) *.o
