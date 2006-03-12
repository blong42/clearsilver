

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ..
endif

include $(NEOTONIC_ROOT)/rules.mk

all: config.save ext/hdf/hdf.so testrb

config.save: install.rb
	$(RUBY) install.rb config -- --with-hdf-include=../../.. --with-hdf-lib=../../../libs --make-prog=$(MAKE)

ext/hdf/Makefile:
	$(RUBY) install.rb config -- --with-hdf-include=../../.. --with-hdf-lib=../../../libs --make-prog=$(MAKE)

ext/hdf/hdf.so: config.save
	$(RUBY) install.rb setup 

gold: ext/hdf/hdf.so
	$(RUBY) -Ilib -Iext/hdf test/hdftest.rb > hdftest.gold;
	@echo "Generated gold files"

testrb: ext/hdf/hdf.so
	@echo "Running ruby test"
	@failed=0; \
	rm -f hdftest.out; \
	$(RUBY) -Ilib -Iext/hdf test/hdftest.rb > hdftest.out; \
	diff --brief hdftest.out hdftest.gold > /dev/null 2>&1; \
	return_code=$$?; \
	if [ $$return_code -ne 0 ]; then \
	  diff hdftest.out hdftest.gold > hdftest.err; \
	  echo "Failed Ruby Test: hdftest.rb"; \
	  echo "    See hdftest.out and hdftest.err"; \
	  failed=1; \
	fi; \
	if [ $$failed -eq 1 ]; then \
	  exit 1; \
	fi;
	@echo  "Passed ruby test"


install: all
	$(RUBY) install.rb install

clean:
	$(RM) ext/hdf/*.o ext/hdf/*.so

distclean:
	$(RM) Makefile.depends config.save ext/hdf/hdf.so
	$(RM) ext/hdf/Makefile ext/hdf/mkmf.log ext/hdf/*.o
