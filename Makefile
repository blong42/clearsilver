
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

clean:
	@for mdir in $(SUBDIRS); do \
	  $(MAKE) -C $$mdir clean; \
	done

clobber:
	@for mdir in $(SUBDIRS); do \
	  $(MAKE) -C $$mdir clobber; \
	done
	@for mdir in $(OUTDIRS); do \
		rm -f $$mdir/*; \
	done

output_dir:
	@for mdir in $(OUTDIRS); do \
		mkdir -p $$mdir; \
	done
