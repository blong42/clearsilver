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

$(streamhtmlparser_dir)/.libs/libstreamhtmlparser.a:
	$(MAKE) -C $(streamhtmlparser_dir) PREFIX=$(prefix) || exit 1;

libs/libstreamhtmlparser.a: $(streamhtmlparser_dir)/.libs/libstreamhtmlparser.a output_dir
	$(CP) $(streamhtmlparser_dir)/.libs/libstreamhtmlparser.a libs/

rules.mk: configure
	./configure

configure: configure.in
	./autogen.sh

cs: libs/libstreamhtmlparser.a output_dir
	@for mdir in $(SUBDIRS); do \
	  if test -d $$mdir; then \
	    if test -f $$mdir/Makefile.PL -a ! -f $$mdir/Makefile; then \
	      cd $$mdir; $(PERL) Makefile.PL PREFIX=$(prefix); cd ..; \
	    fi; \
	    $(MAKE) -C $$mdir PREFIX=$(prefix) || exit 1; \
	    if test -f $$mdir/Makefile.PL; then \
	      $(MAKE) -C $$mdir PREFIX=$(prefix) test || exit 1; \
	    fi; \
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
	$(MAKE) -C $(streamhtmlparser_dir) clean

distclean:
	-@for mdir in $(SUBDIRS); do \
	  $(MAKE) -C $$mdir distclean; \
	done
	-@for mdir in $(OUTDIRS); do \
		rm -rf $$mdir/*; \
	done
	$(MAKE) -C $(streamhtmlparser_dir) distclean
	rm -f config.cache config.log config.status rules.mk cs_config.h
	rm -rf autom4te.cache

output_dir:
	@for mdir in $(OUTDIRS); do \
		mkdir -p $$mdir; \
	done

CS_DISTDIR = clearsilver-0.11.1
CS_LABEL = CLEARSILVER-0_11_1
CS_FILES = README README.python INSTALL LICENSE COPYING rules.mk.in Makefile acconfig.h autogen.sh config.guess config.sub configure.in cs_config.h.in mkinstalldirs install-sh ClearSilver.h
CS_DIRS = util streamhtmlparser cs cgi python scripts mod_ecs imd java perl ruby dso csharp ports contrib m4

testbuildno:
	@echo 0 > testbuildno

cs_test_dist: testbuildno
	@expr `cat testbuildno` + 1 >testbuildno
	@CS_DISTDIR=$(CS_DISTDIR)-rc`cat testbuildno`; \
	rm -rf $$CS_DISTDIR; \
	mkdir -p $$CS_DISTDIR; \
	tar -c --exclude BUILD --exclude experimental -f - $(CS_FILES) $(CS_DIRS) | (cd $$CS_DISTDIR; tar -xf -) ; \
	$(MAKE) -C $$CS_DISTDIR man distclean; \
	chmod -R u+w $$CS_DISTDIR; \
	chmod -R a+r $$CS_DISTDIR; \
	tar chozf $$CS_DISTDIR.tar.gz $$CS_DISTDIR

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
	tar -c --exclude BUILD --exclude experimental -f - `p4 files $(CS_FILES) $(addsuffix /..., $(CS_DIRS)) | cut -d'#' -f 1 | sed -e "s|//depot/google3/third_party/clearsilver/core/||"` | (cd $(CS_DISTDIR); tar -xf -)
	$(MAKE) -C $(CS_DISTDIR) man distclean
	chmod -R u+w $(CS_DISTDIR)
	chmod -R a+r $(CS_DISTDIR)
	tar chozf $(CS_DISTDIR).tar.gz $(CS_DISTDIR)
