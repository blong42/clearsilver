#
# Neotonic Source Kit
#
# Copyright (C) 2001 Neotonic and Brandon Long
#
# This file is licensed under the terms of the FSF's LGPL, or
# Library General Public License.
#


include rules.mk

SUBDIRS = util cs cgi python

OUTDIRS = bin libs

all: output_dir
	@for mdir in $(SUBDIRS); do \
		$(MAKE) -C $$mdir; \
	done

depend:
	@for mdir in $(SUBDIRS); \
		do $(MAKE) -C $$mdir depend; \
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
	@find . -name "Makefile.depends" -print | xargs -i% rm %

.PHONY: man
man:
	@mkdir -p man/man3
	@for mdir in $(SUBDIRS); do \
		scripts/document.py --owner "Neotonic, Inc." --outdir man/man3/ $$mdir/*.h; \
	done

.PHONY: hdf
hdf:
	@mkdir -p docs/hdf
	@for mdir in $(SUBDIRS); do \
		scripts/document.py --hdf --owner "Neotonic, Inc." --outdir docs/hdf/ $$mdir/*.h; \
	done

clean:
	@for mdir in $(SUBDIRS); do \
	  $(MAKE) -C $$mdir clean; \
	done

distclean:
	@for mdir in $(SUBDIRS); do \
	  $(MAKE) -C $$mdir distclean; \
	done
	@for mdir in $(OUTDIRS); do \
		rm -f $$mdir/*; \
	done

output_dir:
	@for mdir in $(OUTDIRS); do \
		mkdir -p $$mdir; \
	done
