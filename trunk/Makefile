#
# Neotonic Source Kit
#
# Copyright (C) 2001 Neotonic and Brandon Long
#
#

NEOTONIC_ROOT = ./

include rules.mk

SUBDIRS = util cs cgi $(BUILD_WRAPPERS)

OUTDIRS = bin libs

# These are blank here... but populated under automated build
VERSION =
RELEASE =

all: cs $(BUILD_WRAPPERS)

rules.mk: configure
	./configure

configure: configure.in
	./autogen.sh

cs: output_dir
	@for mdir in $(SUBDIRS); do \
	  if test -d $$mdir; then \
	    if test -f $$mdir/Makefile.PL -a ! -f $$mdir/Makefile; then \
	      cd $$mdir; $(PERL) Makefile.PL; cd ..; \
	    fi; \
	    $(MAKE) -C $$mdir PREFIX=$(prefix); \
	  fi; \
	done

install: all man
	./mkinstalldirs $(DESTDIR)$(cs_includedir)
	./mkinstalldirs $(DESTDIR)$(bindir)
	./mkinstalldirs $(DESTDIR)$(libdir)
	./mkinstalldirs $(DESTDIR)$(mandir)/man3
	$(INSTALL) -m 644 ClearSilver.h $(DESTDIR)$(cs_includedir)/
	$(INSTALL) -m 644 cs_config.h $(DESTDIR)$(cs_includedir)/
	$(INSTALL) -m 644 man/man3/*.3 $(DESTDIR)$(mandir)/man3/
	@for mdir in $(SUBDIRS); do \
	  if test -d $$mdir; then \
	    if test -f $$mdir/Makefile.PL -a ! -f $$mdir/Makefile; then \
	      cd $$mdir; $(PERL) Makefile.PL; cd ..; \
	    fi; \
	    $(MAKE) -C $$mdir PREFIX=$(prefix) install; \
	  fi; \
	done

depend:
	@for mdir in $(SUBDIRS); do \
	  if test ! -f $$mdir/Makefile.PL; then \
	    $(MAKE) -C $$mdir depend; \
	  fi; \
	done

newdepend: killdepend
	@echo "*******************************************"
	@echo "** Building dependencies..."
	@for mdir in $(SUBDIRS); \
		do $(MAKE) -C $$mdir depend; \
	done

killdepend:
	@echo "*******************************************"
	@echo "** Removing Old dependencies..."
	@find . -name "Makefile.depends" -print | $(XARGS) rm %

.PHONY: man
man:
	@mkdir -p man/man3
	@for mdir in $(SUBDIRS); do \
		scripts/document.py --owner "ClearSilver" --outdir man/man3/ $$mdir/*.h; \
	done

.PHONY: hdf
hdf:
	@mkdir -p docs/hdf
	@for mdir in $(SUBDIRS); do \
		scripts/document.py --hdf --owner "ClearSilver" --outdir docs/hdf/ $$mdir/*.h; \
	done

changelog:
	rcs2log -v | cat - ChangeLog | sed -e "s/\/b2\/src\/cvsroot\/neotonic\///g" > ChangeLog.$$$$ && mv ChangeLog.$$$$ ChangeLog

clean:
	@for mdir in $(SUBDIRS); do \
	  $(MAKE) -C $$mdir clean; \
	done

distclean:
	@for mdir in $(SUBDIRS); do \
	  $(MAKE) -C $$mdir distclean; \
	done
	@for mdir in $(OUTDIRS); do \
		rm -rf $$mdir/*; \
	done
	rm -f config.cache config.log config.status rules.mk cs_config.h

output_dir:
	@for mdir in $(OUTDIRS); do \
		mkdir -p $$mdir; \
	done

CS_DISTDIR = clearsilver-0.9.8
CS_LABEL = CLEARSILVER-0_9_8
CS_FILES = README README.python INSTALL LICENSE CS_LICENSE rules.mk.in Makefile util cs cgi python scripts mod_ecs imd java-jni perl ruby dso csharp acconfig.h autogen.sh config.guess config.sub configure.in cs_config.h.in mkinstalldirs install-sh ClearSilver.h ports contrib
cs_dist:
	@if cvs log Makefile | grep "${CS_LABEL}"; then \
	  echo "release ${CS_LABEL} already exists"; \
	  echo "   to rebuild, type:  cvs tag -d -F ${CS_LABEL} Makefile "; \
	  exit 1; \
	fi; 
	rm -rf $(CS_DISTDIR)
	cvs -q tag -F $(CS_LABEL) $(CS_FILES)
	mkdir -p $(CS_DISTDIR)
	cvs -z3 -q export -r $(CS_LABEL) -d $(CS_DISTDIR) neotonic
	-rm -rf $(CS_DISTDIR)/CVS
	$(MAKE) -C $(CS_DISTDIR) man distclean
	tar chozf $(CS_DISTDIR).tar.gz $(CS_DISTDIR)

trakken: cs
	$(MAKE) -C retrieve
	$(MAKE) VERSION=$(VERSION) RELEASE=$(RELEASE) -C trakken
