

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ..
endif

include $(NEOTONIC_ROOT)/rules.mk

IMD_EXE = imd.cgi
IMD_SRC = imd.c
IMD_OBJ = $(IMD_SRC:%.c=%.o)

CFLAGS += -I/usr/local/include
DLIBS += -lneo_cgi -lneo_cs -lneo_utl # -lefence
LIBS += -L$(LIB_DIR) $(DLIBS) -L/usr/local/lib -lgd -ljpeg -lz

TARGETS = $(IMD_EXE)

all: $(TARGETS)

$(IMD_EXE): $(IMD_OBJ) $(DEP_LIBS)
	$(LD) $@ $(IMD_OBJ) $(LIBS)

clean:
	$(RM) *.o

distclean:
	$(RM) Makefile.depends $(TARGETS) *.o
