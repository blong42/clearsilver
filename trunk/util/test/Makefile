

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ../../
endif

include $(NEOTONIC_ROOT)rules.mk

CFLAGS += -I$(NEOTONIC_ROOT)/util
LIBS += -lneo_utl 

DLIB = $(LIB_DIR)libneo_utl.a


# A simple test is one where there is a single .c file which compiles to
# a binary linked against the normal libs
SIMPLE_TESTS = date_test hash_test hdf_copy_test hdf_dealloc_test \
	       hdf_sort_test hdf_load_test hdf_test listdir_test net_test \
	       ulist_test neo_err_test

TARGETS = $(SIMPLE_TESTS)

all: $(TARGETS)

$(SIMPLE_TESTS): %_test: %_test.o
	$(LD) $@ $< $(LDFLAGS) $(LIBS)

clean:
	$(RM) *.o

distclean:
	$(RM) $(TARGETS) *.o
