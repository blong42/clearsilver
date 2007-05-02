

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ..
endif

include $(NEOTONIC_ROOT)/rules.mk

CS_LIB = $(LIB_DIR)libneo_cs.a
CS_SRC = csparse.c
CS_OBJ = $(CS_SRC:%.c=%.o)

CSTEST_EXE = cstest
CSTEST_SRC = cstest.c
CSTEST_OBJ = $(CSTEST_SRC:%.c=%.o)

CSR_EXE = cs
CSR_SRC = cs.c
CSR_OBJ = $(CSR_SRC:%.c=%.o)

CSDUMP_EXE = csdump
CSDUMP_SRC = csdump.c
CSDUMP_OBJ = $(CSDUMP_SRC:%.c=%.o)

LIBS += -lneo_cs -lneo_utl #  -lefence

TARGETS = $(CS_LIB) $(CSTEST_EXE) $(CSR_EXE) test

CS_TESTS = test.cs test2.cs test3.cs test4.cs test5.cs test6.cs test7.cs \
           test8.cs test9.cs test10.cs test11.cs test12.cs test13.cs \
	   test14.cs test15.cs test16.cs test17.cs test18.cs test19.cs \
	   test_var.cs test_paren.cs test_chuck.cs test_trak1.cs test_iter.cs \
	   test_each_array.cs test_name.cs test_with.cs test_numbers.cs \
	   test_splice.cs test_joo.cs test_first_last.cs test_abs_max_min.cs \
	   test_comma.cs test_macro_set.cs test_func.cs test_escape.cs \
	   test_uvar.cs test_crc.cs


all: $(TARGETS)

$(CS_LIB): $(CS_OBJ)
	$(AR) $@ $(CS_OBJ)
	$(RANLIB) $@

$(CSTEST_EXE): $(CSTEST_OBJ) $(CS_LIB)
	$(LD) $@ $(CSTEST_OBJ) $(LDFLAGS) $(LIBS) # -lefence

$(CSR_EXE): $(CSR_OBJ) $(CS_LIB)
	$(LD) $@ $(CSR_OBJ) $(LDFLAGS) $(LIBS) # -lefence

$(CSDUMP_EXE): $(CSDUMP_OBJ) $(CS_LIB)
	$(LD) $@ $(CSDUMP_OBJ) $(LDFLAGS) $(LIBS)

## BE VERY CAREFUL WHEN REGENERATING THESE
gold: $(CSTEST_EXE)
	@for test in $(CS_TESTS); do \
		rm -f $$test.gold; \
		./cstest test.hdf $$test > $$test.gold; \
	done; \
	./cstest test_tag.hdf test_tag.cs > test_tag.cs.gold
	@echo "Generated Gold Files"

test: $(CSTEST_EXE) $(CS_TESTS) 
	@echo "Running cs regression tests"
	@failed=0; \
	for test in $(CS_TESTS); do \
		rm -f $$test.out; \
		./cstest test.hdf $$test > $$test.out 2>&1; \
		diff $$test.out $$test.gold 2>&1 > /dev/null; \
		return_code=$$?; \
		if [ $$return_code -ne 0 ]; then \
		  diff $$test.gold $$test.out > $$test.err; \
		  echo "Failed Regression Test: $$test"; \
		  echo "  See $$test.out and $$test.err"; \
		  failed=1; \
		fi; \
	done; \
	rm -f test_tag.cs.out; \
	./cstest test_tag.hdf test_tag.cs> test_tag.cs.out 2>&1; \
	diff test_tag.cs.out test_tag.cs.gold; \
	return_code=$$?; \
	if [ $$return_code -ne 0 ]; then \
	  echo "Failed Regression Test: test_tag.cs"; \
	  failed=1; \
	fi; \
	if [ $$failed -eq 1 ]; then \
	  exit 1; \
	fi;
	@touch test
	@echo "Passed"
	
install: all
	$(NEOTONIC_ROOT)/mkinstalldirs $(DESTDIR)$(cs_includedir)/cs
	$(INSTALL) -m 644 cs.h $(DESTDIR)$(cs_includedir)/cs
	$(INSTALL) -m 644 $(CS_LIB) $(DESTDIR)$(libdir)
	$(INSTALL) $(CSTEST_EXE) $(DESTDIR)$(bindir)
	$(INSTALL) $(CSR_EXE) $(DESTDIR)$(bindir)

clean:
	$(RM) core *.o

distclean:
	$(RM) Makefile.depends $(TARGETS) core *.o *.out
