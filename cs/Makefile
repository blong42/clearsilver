

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../
endif

include $(NEOTONIC_ROOT)rules.mk

CS_LIB = $(LIB_DIR)libneo_cs.a
CS_SRC = csparse.c
CS_OBJ = $(CS_SRC:%.c=%.o)

CSTEST_EXE = cstest
CSTEST_SRC = cstest.c
CSTEST_OBJ = $(CSTEST_SRC:%.c=%.o)

CSDUMP_EXE = csdump
CSDUMP_SRC = csdump.c
CSDUMP_OBJ = $(CSDUMP_SRC:%.c=%.o)

CFLAGS += -I$(NEOTONIC_ROOT)
LIBS += -L$(LIB_DIR) -lneo_cs -lneo_utl # -lefence

TARGETS = $(CS_LIB) $(CSTEST_EXE) test

CS_TESTS = test.cs test2.cs test3.cs test4.cs test5.cs test6.cs

all: $(TARGETS)

$(CS_LIB): $(CS_OBJ)
	$(AR) $@ $(CS_OBJ)

$(CSTEST_EXE): $(CSTEST_OBJ) $(CS_LIB)
	$(LD) $@ $(CSTEST_OBJ) $(LIBS)

$(CSDUMP_EXE): $(CSDUMP_OBJ) $(CS_LIB)
	$(LD) $@ $(CSDUMP_OBJ) $(LIBS)

## BE VERY CAREFUL WHEN REGENERATING THESE
gold: $(CSTEST_EXE)
	@for test in $(CS_TESTS); do \
		rm -f $$test.gold; \
		./cstest test.hdf $$test > $$test.gold; \
	done

test: $(CSTEST_EXE) $(CS_TESTS)
	@echo "Running cs regression tests"
	@for test in $(CS_TESTS); do \
		rm -f $$test.out; \
		./cstest test.hdf $$test > $$test.out; \
		diff --brief $$test.out $$test.gold; \
		return_code=$$?; \
		if [ $$return_code -ne 0 ]; then \
		  echo "Failed Regression Test: $$test.out"; \
		  exit 1; \
		fi; \
	done
	@touch test
	@echo "Passed"
	

clean:
	$(RM) core *.o

distclean:
	$(RM) $(TARGETS) core *.o
