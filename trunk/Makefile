#
# Neotonic Source Kit
#
# Copyright (C) 2001 Neotonic and Brandon Long
#
#

NEOTONIC_ROOT = .

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
	      cd $$mdir; $(PERL) Makefile.PL PREFIX=$(prefix); cd ..; \
	    fi; \
	    $(MAKE) -C $$mdir PREFIX=$(prefix); \
	  fi; \
	done

install: all
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
	      cd $$mdir; $(PERL) Makefile.PL PREFIX=$(prefix); cd ..; \
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
	p4 changes -l ./...
	

clean:
	-@for mdir in $(SUBDIRS); do \
	  $(MAKE) -C $$mdir clean; \
	done

distclean:
	-@for mdir in $(SUBDIRS); do \
	  $(MAKE) -C $$mdir distclean; \
	done
	-@for mdir in $(OUTDIRS); do \
		rm -rf $$mdir/*; \
	done
	rm -f config.cache config.log config.status rules.mk cs_config.h
	rm -rf autom4te.cache

output_dir:
	@for mdir in $(OUTDIRS); do \
		mkdir -p $$mdir; \
	done

CS_DISTDIR = clearsilver-0.10.5
CS_LABEL = CLEARSILVER-0_10_5
CS_FILES = README README.python INSTALL LICENSE CS_LICENSE rules.mk.in Makefile acconfig.h autogen.sh config.guess config.sub configure.in cs_config.h.in mkinstalldirs install-sh ClearSilver.h
CS_DIRS = util cs cgi python scripts mod_ecs imd java-jni perl ruby dso csharp ports contrib m4

cs_dist:
	@if p4 labels Makefile | grep "${CS_LABEL}"; then \
	  echo "release ${CS_LABEL} already exists"; \
	  echo "   to rebuild, type:  p4 label -d ${CS_LABEL}"; \
	  exit 1; \
	fi; 
	rm -rf $(CS_DISTDIR)
	p4 label $(CS_LABEL)
	p4 labelsync -l$(CS_LABEL) $(CS_FILES) $(addsuffix /..., $(CS_DIRS))
	mkdir -p $(CS_DISTDIR)
	tar -cf - `p4 files $(CS_FILES) $(addsuffix /..., $(CS_DIRS)) | cut -d'#' -f 1 | sed -e "s|//depot/google3/third_party/clearsilver/core/||"` | (cd $(CS_DISTDIR); tar -xf -)
	$(MAKE) -C $(CS_DISTDIR) man distclean
	chmod -R u+w $(CS_DISTDIR)
	chmod -R a+r $(CS_DISTDIR)
	tar chozf $(CS_DISTDIR).tar.gz $(CS_DISTDIR)
