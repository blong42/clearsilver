

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ..
endif

include $(NEOTONIC_ROOT)/rules.mk

MOD_ECS_SRC = mod_ecs.c
MOD_ECS_SO = mod_ecs.so

DLIBS += -lneo_cgi -lneo_cs -lneo_utl 
LIBS += $(DLIBS) $(DB2_LIB)

TARGETS = $(MOD_ECS_SO)

all: $(TARGETS)

$(MOD_ECS_SO): $(MOD_ECS_SRC) $(DEP_LIBS)
	$(APXS) -c -o $@ $(MOD_ECS_SRC)

install: all
	$(NEOTONIC_ROOT)/mkinstalldirs $(DESTDIR)$(libexecdir)
	$(INSTALL) $(MOD_ECS_SO) $(DESTDIR)$(libexecdir)

# Hmm, install it in the default apache place, or in with the 
# rest of our stuff?
#	$(APXS) -i $(MOD_ECS_SO)

clean:
	$(RM) *.o

distclean:
	$(RM) Makefile.depends $(TARGETS) *.o
