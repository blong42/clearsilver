/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the FSF's
 * Library Gnu Public License (LGPL).
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef __NEO_HDF_H_
#define __NEO_HDF_H_ 1

__BEGIN_DECLS

#include <stdio.h>
#include "neo_err.h"
#include "ulist.h"

typedef struct _hdf
{
  int top;
  int alloc_value;
  char *name;
  int name_len;
  char *value;
  struct _hdf *next;
  struct _hdf *child;
} HDF;

NEOERR* hdf_init (HDF **hdf);
void hdf_destroy (HDF **hdf);

int hdf_get_int_value (HDF *hdf, char *name, int defval);
char *hdf_get_value (HDF *hdf, char *name, char *defval);
NEOERR* hdf_get_copy (HDF *hdf, char *name, char **value, char *defval);

HDF* hdf_get_obj (HDF *hdf, char *name);
HDF* hdf_get_child (HDF *hdf, char *name);
HDF* hdf_obj_child (HDF *hdf);
HDF* hdf_obj_next (HDF *hdf);
char* hdf_obj_name (HDF *hdf);
char* hdf_obj_value (HDF *hdf);

NEOERR* hdf_set_value (HDF *hdf, char *name, char *value);
NEOERR* hdf_set_int_value (HDF *hdf, char *name, int value);
NEOERR* hdf_set_copy (HDF *hdf, char *dest, char *src);
NEOERR* hdf_set_buf (HDF *hdf, char *name, char *value);

NEOERR* hdf_read_file (HDF *hdf, char *path);
NEOERR* hdf_write_file (HDF *hdf, char *path);

NEOERR* hdf_dump (HDF *hdf, char *prefix);
NEOERR* hdf_dump_format (HDF *hdf, int lvl, FILE *fp);
NEOERR* hdf_dump_str(HDF *hdf, char *prefix, STRING *str);

NEOERR* hdf_remove_tree (HDF *hdf, char *name);

/* Utility function */
NEOERR* hdf_search_path (HDF *hdf, char *path, char *full);

__END_DECLS

#endif /* __NEO_HDF_H_ */
