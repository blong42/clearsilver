

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../
endif

include $(NEOTONIC_ROOT)rules.mk

CGI_LIB = $(LIB_DIR)libneo_cgi.a
CGI_SRC = cgiwrap.c cgi.c html.c
CGI_OBJ = $(CGI_SRC:%.c=%.o)

STATIC_EXE = static.cgi
STATIC_SRC = static.c
STATIC_OBJ = $(STATIC_SRC:%.c=%.o)

CFLAGS += -I$(NEOTONIC_ROOT)
DLIBS += -lneo_cgi -lneo_cs -lneo_utl # -lefence
LIBS += -L$(LIB_DIR) $(DLIBS)

TARGETS = $(CGI_LIB) $(STATIC_EXE)

all: $(TARGETS)

$(CGI_LIB): $(CGI_OBJ)
	$(AR) $@ $(CGI_OBJ)

$(STATIC_EXE): $(STATIC_OBJ) $(DEP_LIBS)
	$(LD) $@ $(STATIC_OBJ) $(LIBS)

clean:
	$(RM) *.o

distclean:
	$(RM) $(TARGETS) *.o
