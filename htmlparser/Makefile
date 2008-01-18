ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ..
endif

include $(NEOTONIC_ROOT)/rules.mk

HTML_LIB = $(LIB_DIR)libneo_htmlparser.a
HTML_SRC = statemachine.c htmlparser.c jsparser.c 

HTML_OBJ = $(HTML_SRC:%.c=%.o)
HTML_HDR = $(HTML_SRC:%.c=%.h)

HTML_TEST_EXE = test2
HTML_TEST_SRC = test2.c
HTML_TEST_OBJ = $(HTML_TEST_SRC:%.c=%.o)

TARGETS = $(HTML_LIB)

all: $(TARGETS)

$(HTML_LIB): $(HTML_OBJ)
	$(AR) $@ $(HTML_OBJ)
	$(RANLIB) $@

install: all
	$(NEOTONIC_ROOT)/mkinstalldirs $(DESTDIR)$(cs_includedir)/htmlparser
	$(INSTALL) -m 644 $(HTML_HDR) $(DESTDIR)$(cs_includedir)/htmlparser
	$(INSTALL) -m 644 $(HTML_LIB) $(DESTDIR)$(libdir)

clean:
	$(RM) *.o

distclean:
	$(RM) Makefile.depends $(TARGETS) *.o
