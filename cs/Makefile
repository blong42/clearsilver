

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

CSTEST_AUTO_EXE = cstest_auto
CSTEST_AUTO_SRC = cstest_auto.c
CSTEST_AUTO_OBJ = $(CSTEST_AUTO_SRC:%.c=%.o)

CSR_EXE = cs
CSR_SRC = cs.c
CSR_OBJ = $(CSR_SRC:%.c=%.o)

CSDUMP_EXE = csdump
CSDUMP_SRC = csdump.c
CSDUMP_OBJ = $(CSDUMP_SRC:%.c=%.o)

DLIBS += -lneo_cs -lneo_utl -lstreamhtmlparser #  -lefence

TARGETS = $(CS_LIB) $(CSTEST_EXE) $(CSR_EXE) $(CSTEST_AUTO_EXE) test

CS_TESTS = test.cs test2.cs test3.cs test4.cs test5.cs test6.cs test7.cs \
           test8.cs test9.cs test10.cs test11.cs test12.cs test13.cs \
	   test14.cs test15.cs test16.cs test17.cs test18.cs test19.cs \
	   test_var.cs test_paren.cs test_chuck.cs test_trak1.cs test_iter.cs \
	   test_each_array.cs test_name.cs test_with.cs test_numbers.cs \
	   test_splice.cs test_joo.cs test_first_last.cs test_abs_max_min.cs \
	   test_comma.cs test_macro_set.cs test_func.cs test_escape.cs \
	   test_uvar.cs test_crc.cs test_type.cs test_macro_recursion.cs \
	   test_str_tolower.cs test_macro_set_child.cs test_two_vars_same_ref.cs \
	   test_linclude_macro.cs test_multi_arg_scoping.cs \
	   test_local_var_not_losing_child.cs test_set_string_arg.cs \
	   test_global_set.cs test_null_string_add.cs \
	   test_evar_using_global_hdf.cs test_set_null_lvalue.cs

CS_FAILING_TESTS = test_macro_recursion_failing.cs \
		   test_include_recursion_failing.cs \
                   test_linclude_recursion_failing.cs \
                   test_calling_lincluded_macro_failing.cs \
		   test_mutating_global_hdf.cs

all: $(TARGETS)

$(CS_LIB): $(CS_OBJ)
	$(AR) $@ $(CS_OBJ)
	$(RANLIB) $@

$(CSTEST_EXE): $(CSTEST_OBJ) $(CS_LIB)
	$(LD) $@ $(CSTEST_OBJ) $(LDFLAGS) $(DLIBS) # -lefence

$(CSTEST_AUTO_EXE): $(CSTEST_AUTO_OBJ) $(CS_LIB)
	$(LD) $@ $(CSTEST_AUTO_OBJ) $(LDFLAGS) $(DLIBS) # -lefence

$(CSR_EXE): $(CSR_OBJ) $(CS_LIB)
	$(LD) $@ $(CSR_OBJ) $(LDFLAGS) $(DLIBS) # -lefence

$(CSDUMP_EXE): $(CSDUMP_OBJ) $(CS_LIB)
	$(LD) $@ $(CSDUMP_OBJ) $(LDFLAGS) $(DLIBS)

## BE VERY CAREFUL WHEN REGENERATING THESE
gold: $(CSTEST_EXE) $(CSTEST_AUTO_EXE)
	@for test in $(CS_TESTS); do \
		rm -f $$test.gold; \
		./cstest -global_hdf global_test.hdf test.hdf $$test > $$test.gold; \
	done; \
	for test in $(CS_FAILING_TESTS); do \
		rm -f $$test.gold; \
		./cstest -global_hdf global_test.hdf -parse_must_fail test.hdf $$test > $$test.gold; \
	done; \
	rm -f test_html.cs.gold \
	./$(CSTEST_AUTO_EXE) test.hdf test_html.cs > test_html.cs.gold; \
	./cstest test_tag.hdf test_tag.cs > test_tag.cs.gold
	@echo "Generated Gold Files"

test: $(CSTEST_EXE) $(CSTEST_AUTO_EXE) $(CS_TESTS) $(CS_FAILING_TESTS) \
      test_html.cs
	@echo "Running cs regression tests"
	@failed=0; \
	for test in $(CS_TESTS); do \
		rm -f $$test.out; \
		./cstest -global_hdf global_test.hdf test.hdf $$test > $$test.out 2>&1; \
		diff $$test.out $$test.gold 2>&1 > /dev/null; \
		return_code=$$?; \
		if [ $$return_code -ne 0 ]; then \
		  diff $$test.gold $$test.out > $$test.err; \
		  echo "Failed Regression Test: $$test"; \
		  echo "  See $$test.out and $$test.err"; \
		  failed=1; \
		fi; \
	done; \
	for test in $(CS_FAILING_TESTS); do \
		rm -rf $$test.out; \
		./cstest -global_hdf global_test.hdf -parse_must_fail test.hdf $$test > $$test.out 2>&1; \
		return_code=$$?; \
		if [ $$return_code -ne 0 ]; then \
			diff $$test.gold $$test.out > $$test.err; \
		  echo "Problem in suppposed Failing Test: $$test"; \
		  echo "  See $$test.out and $$test.err"; \
		  failed=1; \
		fi; \
		diff $$test.out $$test.gold 2>&1 > /dev/null; \
		return_code=$$?; \
		if [ $$return_code -ne 0 ]; then \
		  diff $$test.gold $$test.out > $$test.err; \
		  echo "Failed Regression Test: $$test"; \
		  echo "  See $$test.out and $$test.err"; \
		  failed=1; \
		fi; \
	done; \
	rm -f test_html.cs.out; \
	./$(CSTEST_AUTO_EXE) -h test.hdf -c test_html.cs > test_html.cs.out 2>&1; \
	diff test_html.cs.out test_html.cs.gold 2>&1 > /dev/null; \
	return_code=$$?; \
	if [ $$return_code -ne 0 ]; then \
	  diff test_html.cs.gold test_html.cs.out > test_html.cs.err; \
	  echo "Failed Regression Test: $(CSTEST_AUTO_EXE)"; \
	  echo "  See test_html.cs.out and test_html.cs.err"; \
	  failed=1; \
	fi; \
	./$(CSTEST_AUTO_EXE) -t > /dev/null; \
	return_code=$$?; \
	if [ $$return_code -ne 0 ]; then \
	  echo "Failed Regression Test: $(CSTEST_AUTO_EXE)"; \
	  failed=1; \
	fi; \
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

cleantests:
	$(RM) test*.out test*.err
