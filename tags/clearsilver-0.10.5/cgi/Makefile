

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ..
endif

include $(NEOTONIC_ROOT)/rules.mk

CGI_LIB = $(LIB_DIR)libneo_cgi.a
CGI_SRC = cgiwrap.c cgi.c html.c date.c rfc2388.c
CGI_OBJ = $(CGI_SRC:%.c=%.o)

STATIC_EXE = cs_static.cgi
STATIC_SRC = static.c
STATIC_OBJ = $(STATIC_SRC:%.c=%.o)
STATIC_CSO = $(STATIC_EXE:%.cgi=%.cso)

CGICSTEST_EXE = cgi_cstest
CGICSTEST_SRC = cgi_cstest.c
CGICSTEST_OBJ = $(CGICSTEST_SRC:%.c=%.o)

DLIBS += -lneo_cgi -lneo_cs -lneo_utl # -lefence

TARGETS = $(CGI_LIB) $(STATIC_EXE) $(STATIC_CSO) $(CGICSTEST_EXE)

all: $(TARGETS)

$(CGI_LIB): $(CGI_OBJ)
	$(AR) $@ $(CGI_OBJ)
	$(RANLIB) $@

$(STATIC_EXE): $(STATIC_OBJ) $(DEP_LIBS)
	$(LD) $@ $(STATIC_OBJ) $(LDFLAGS) $(DLIBS) $(LIBS)

$(STATIC_CSO): $(STATIC_OBJ) $(DEP_LIBS)
	$(LDSHARED) -o $@ $(STATIC_OBJ) $(LDFLAGS) $(DLIBS) $(LIBS)

$(CGICSTEST_EXE): $(CGICSTEST_OBJ) $(DEP_LIBS)
	$(LD) $@ $(CGICSTEST_OBJ) $(LDFLAGS) $(DLIBS) $(LIBS)

install: all
	$(NEOTONIC_ROOT)/mkinstalldirs $(DESTDIR)$(cs_includedir)/cgi
	$(INSTALL) -m 644 cgi.h $(DESTDIR)$(cs_includedir)/cgi
	$(INSTALL) -m 644 cgiwrap.h $(DESTDIR)$(cs_includedir)/cgi
	$(INSTALL) -m 644 date.h $(DESTDIR)$(cs_includedir)/cgi
	$(INSTALL) -m 644 html.h $(DESTDIR)$(cs_includedir)/cgi
	$(INSTALL) -m 644 $(CGI_LIB) $(DESTDIR)$(libdir)
	$(INSTALL) $(STATIC_EXE) $(DESTDIR)$(bindir)

clean:
	$(RM) *.o

distclean:
	$(RM) Makefile.depends $(TARGETS) *.o
