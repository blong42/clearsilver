##
## Global Makefile Rules
##
## Before including this file, you must set NEOTONIC_ROOT
##

OSNAME := $(shell uname -rs | cut -f 1-2 -d "." | cut -f 1 -d "-")
OSTYPE := $(shell uname -s)

LIB_DIR    = $(NEOTONIC_ROOT)libs/

## NOTE: The wdb code in util will tickle a bug in SleepyCat 2.4.5,
## which ships with various versions of Linux as part of glibc.  If you
## are going to use that code, you should compile against SleepyCat
## 2.7.7 instead
USE_DB2 = 1

USE_ZLIB = 1

PYTHON_INC = -I/neo/opt/include/python2.2

## Programs
MKDIR      = mkdir -p
RM         = rm -f
CC         = gcc
CPP        = g++

CFLAGS     = -g -O2 -Wall -c -I$(NEOTONIC_ROOT)  -I/neo/opt/include 
OUTPUT_OPTION = -o $@
LD         = $(CC) -o
LDFLAGS    = -L$(LIB_DIR)
LDSHARED   = $(CC) -shared -fPi
CPPLDSHARED   = $(CPP) -shared -fPic
AR         = ar -cr
DEP_LIBS   = $(DLIBS:-l%=$(LIB_DIR)lib%.a)
LIBS       =
LS         = /bin/ls

ifeq ($(OSNAME),WindowsNT 0)
CFLAGS += -D__WINDOWS_GCC__
USE_DB2 = 0
USE_ZLIB = 0
SHELL=cmd.exe
LIBS += -lmingwex -Lc:/local/lib
LS = ls
endif

ifeq ($(USE_ZLIB),1)
LIBS += -lz
endif

ifeq ($(USE_DB2),1)
DB2_INC = -I$(HOME)/src/db-2.7.7/dist
DB2_LIB = -L$(HOME)/src/db-2.7.7/dist -ldb
CFLAGS += $(DB2_INC)
endif

.c.o:
	$(CC) $(CFLAGS) $(OUTPUT_OPTION) $<

everything: depend all

.PHONY: depend
depend: Makefile.depends

Makefile.depends: $(NEOTONIC_ROOT)/rules.mk Makefile
	@echo "*******************************************"
	@echo "** Building Dependencies "
	@rm -f Makefile.depends
	@touch Makefile.depends
	@for II in `$(LS) -1 *.c`; do \
		gcc -M -MG ${CFLAGS} $$II >> Makefile.depends; \
	done;

DEPEND_FILE := $(shell find . -name Makefile.depends -print)
ifneq ($(DEPEND_FILE),)
include Makefile.depends
endif
