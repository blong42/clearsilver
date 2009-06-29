ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ..
endif

include $(NEOTONIC_ROOT)/rules.mk

HTML_LIB = $(LIB_DIR)libneo_htmlparser.a
HTML_SRC = statemachine.c htmlparser.c jsparser.c 

HTML_OBJ = $(HTML_SRC:%.c=%.o)
HTML_HDR = $(HTML_SRC:%.c=%.h)

HTMLPARSER_TEST_EXE = htmlparser_test
HTMLPARSER_TEST_SRC = htmlparser_test.c
HTMLPARSER_TEST_OBJ = $(HTMLPARSER_TEST_SRC:%.c=%.o)

JSPARSER_TEST_EXE = jsparser_test
JSPARSER_TEST_SRC = jsparser_test.c
JSPARSER_TEST_OBJ = $(JSPARSER_TEST_SRC:%.c=%.o)

STATEMACHINE_TEST_EXE = statemachine_test
STATEMACHINE_TEST_SRC = statemachine_test.c
STATEMACHINE_TEST_OBJ = $(STATEMACHINE_TEST_SRC:%.c=%.o)

GENERATE_FSM = tools/generate_fsm.py
FSM_CONFIG = tools/fsm_config.py
FSMS = htmlparser_fsm jsparser_fsm statemachine_test_fsm

TARGETS = $(HTML_LIB) test

all: $(TARGETS)

$(HTML_LIB): $(HTML_OBJ)
	$(AR) $@ $(HTML_OBJ)
	$(RANLIB) $@

$(HTMLPARSER_TEST_EXE): $(HTMLPARSER_TEST_OBJ) $(HTML_LIB)
	$(LD) $@ $(HTMLPARSER_TEST_OBJ) $(LDFLAGS) $(HTML_LIB)

$(JSPARSER_TEST_EXE): $(JSPARSER_TEST_OBJ) $(HTML_LIB)
	$(LD) $@ $(JSPARSER_TEST_OBJ) $(LDFLAGS) $(HTML_LIB)

$(STATEMACHINE_TEST_EXE): $(STATEMACHINE_TEST_OBJ) $(HTML_LIB)
	$(LD) $@ $(STATEMACHINE_TEST_OBJ) $(LDFLAGS) $(HTML_LIB)

generate_fsms: $(GENERATE_FSM) $(FSM_CONFIG)
	@for test in $(FSMS); do \
		rm -f $$test.h; \
		$(GENERATE_FSM) $$test.config > $$test.h; \
	done;
	@echo "Generated fsm header files"

install: all
	$(NEOTONIC_ROOT)/mkinstalldirs $(DESTDIR)$(cs_includedir)/streamhtmlparser
	$(INSTALL) -m 644 $(HTML_HDR) $(DESTDIR)$(cs_includedir)/streamhtmlparser
	$(INSTALL) -m 644 $(HTML_LIB) $(DESTDIR)$(libdir)

test: $(HTMLPARSER_TEST_EXE) $(JSPARSER_TEST_EXE) $(STATEMACHINE_TEST_EXE)
	@echo "Running HTML parser unit tests"
	@./$(HTMLPARSER_TEST_EXE) ; \
	return_code=$$?; \
	if [ $$return_code -ne 0 ]; then \
	  echo "Failed Test $(HTMLPARSER_TEST_EXE)"; \
	  exit 1; \
	fi;
	@echo "Running JS parser unit tests"
	@./$(JSPARSER_TEST_EXE) ; \
	return_code=$$?; \
	if [ $$return_code -ne 0 ]; then \
	  echo "Failed Test $(JSPARSER_TEST_EXE)"; \
	  exit 1; \
	fi;
	@echo "Running Statemachine unit tests"
	@./$(STATEMACHINE_TEST_EXE) ; \
	return_code=$$?; \
	if [ $$return_code -ne 0 ]; then \
	  echo "Failed Test $(STATEMACHINE_TEST_EXE)"; \
	  exit 1; \
	fi;
	@echo "Passed"

clean:
	$(RM) *.o $(HTMLPARSER_TEST_EXE) $(JSPARSER_TEST_EXE) $(STATEMACHINE_TEST_EXE)

distclean: clean
	$(RM) Makefile.depends $(TARGETS) *.o
