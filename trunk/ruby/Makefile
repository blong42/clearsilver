

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../
endif

include $(NEOTONIC_ROOT)rules.mk

all: config.save ext/hdf/hdf.so test

config.save: install.rb
	$(RUBY) install.rb config -- --with-hdf-include=../../.. --with-hdf-lib=../../../libs

ext/hdf/Makefile:
	$(RUBY) install.rb config -- --with-hdf-include=../../.. --with-hdf-lib=../../../libs

ext/hdf/hdf.so: config.save
	$(RUBY) install.rb setup 

test: ext/hdf/hdf.so
	$(RUBY) -Ilib -Iext/hdf test/hdftest.rb

install: all

clean:
	$(RM) ext/hdf/*.o ext/hdf/*.so

distclean:
	$(RM) Makefile.depends config.save ext/hdf/hdf.so
	$(RM) ext/hdf/Makefile ext/hdf/mkmf.log ext/hdf/*.o
