##
## Global Makefile Rules
##
## Before including this file, you must set NEOTONIC_ROOT
##

OSNAME := $(shell uname -rs | cut -f 1-2 -d "." | cut -f 1 -d "-")

LIB_DIR    = $(NEOTONIC_ROOT)libs/

## Programs
MKDIR      = mkdir -p
RM         = rm -f
FLEX       = flex
CC         = gcc
CFLAGS     = -g -O2 -Wall -c -I$(NEOTONIC_ROOT)include -I$(NEOTONIC_ROOT)util
OUTPUT_OPTION = -o $@
LD         = gcc -o
LDFLAGS    = -L$(LIB_DIR)
AR         = $(MKDIR) $(LIB_DIR); ar -cr

.c.o:
	$(CC) $(CFLAGS) $(OUTPUT_OPTION) $<

