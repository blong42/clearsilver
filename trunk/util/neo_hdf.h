/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef __NEO_HDF_H_
#define __NEO_HDF_H_ 1

__BEGIN_DECLS

#include <stdio.h>
#include "neo_err.h"
#include "neo_hash.h"
#include "ulist.h"

#define FORCE_HASH_AT 10

typedef struct _attr
{
  char *key;
  char *value;
  struct _attr *next;
} HDF_ATTR;

typedef struct _hdf
{
  int link;
  int alloc_value;
  char *name;
  int name_len;
  char *value;
  struct _attr *attr;
  struct _hdf *top;
  struct _hdf *next;
  struct _hdf *child;

  /* the following fields are used to implement a cache */
  struct _hdf *last_hp;
  struct _hdf *last_hs;

  /* the following HASH is used when we reach more than FORCE_HASH_AT
   * elements */
  NE_HASH *hash;
  /* When using the HASH, we need to know where to append new children */
  struct _hdf *last_child;
} HDF;

NEOERR* hdf_init (HDF **hdf);
void hdf_destroy (HDF **hdf);

int hdf_get_int_value (HDF *hdf, char *name, int defval);
char *hdf_get_value (HDF *hdf, char *name, char *defval);
NEOERR* hdf_get_copy (HDF *hdf, char *name, char **value, char *defval);

HDF* hdf_get_obj (HDF *hdf, char *name);
HDF* hdf_get_child (HDF *hdf, char *name);
HDF_ATTR* hdf_get_attr (HDF *hdf, char *name);
NEOERR* hdf_set_attr (HDF *hdf, char *name, char *key, char *value);
HDF* hdf_obj_child (HDF *hdf);
HDF* hdf_obj_next (HDF *hdf);
HDF* hdf_obj_top (HDF *hdf);
HDF_ATTR* hdf_obj_attr (HDF *hdf);
char* hdf_obj_name (HDF *hdf);
char* hdf_obj_value (HDF *hdf);

NEOERR* hdf_set_value (HDF *hdf, char *name, char *value);
NEOERR* hdf_set_int_value (HDF *hdf, char *name, int value);
NEOERR* hdf_set_copy (HDF *hdf, char *dest, char *src);
NEOERR* hdf_set_buf (HDF *hdf, char *name, char *value);

NEOERR *hdf_set_symlink (HDF *hdf, char *src, char *dest);

/*
 * Function: hdf_sort_obj - sort the children of an HDF node 
 * Description: hdf_sort_obj will sort the children of an HDF node,
 *              based on the given comparison function.
 * Input: h - HDF node
 *        compareFunc - function which returns 1,0,-1 depending on some 
 *                      criteria.  Given two children of h
 * Output: None (h children will be sorted)
 * Return: None
 */
NEOERR *hdf_sort_obj(HDF *h, int (*compareFunc)(const void *, const void *));

NEOERR* hdf_read_file (HDF *hdf, char *path);
NEOERR* hdf_write_file (HDF *hdf, char *path);
NEOERR* hdf_write_file_atomic (HDF *hdf, char *path);

NEOERR* hdf_read_string (HDF *hdf, char *s);
NEOERR* hdf_read_string_ignore (HDF *hdf, char *s, int ignore);
NEOERR* hdf_write_string (HDF *hdf, char **s);

NEOERR* hdf_dump (HDF *hdf, char *prefix);
NEOERR* hdf_dump_format (HDF *hdf, int lvl, FILE *fp);
NEOERR* hdf_dump_str(HDF *hdf, char *prefix, int compact, STRING *str);

NEOERR* hdf_remove_tree (HDF *hdf, char *name);
NEOERR* hdf_copy (HDF *dest_hdf, char *name, HDF *src);

/* Utility function */
NEOERR* hdf_search_path (HDF *hdf, char *path, char *full);

__END_DECLS

#endif /* __NEO_HDF_H_ */
