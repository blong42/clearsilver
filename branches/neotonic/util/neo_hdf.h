/* 
 * Copyright (C) 2000  Neotonic
 *
 * All Rights Reserved.
 */

#ifndef __NEO_HDF_H_
#define __NEO_HDF_H_ 1

__BEGIN_DECLS

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

NEOERR* hdf_get_int_value (HDF *hdf, char *name, int *value, int defval);
NEOERR* hdf_get_value (HDF *hdf, char *name, char **value, char *defval);
NEOERR* hdf_get_copy (HDF *hdf, char *name, char **value, char *defval);

NEOERR* hdf_get_obj (HDF *hdf, char *name, HDF **obj);
NEOERR* hdf_obj_child (HDF *hdf, HDF **obj);
NEOERR* hdf_obj_next (HDF *hdf, HDF **obj);
NEOERR* hdf_obj_name (HDF *hdf, char **name);
NEOERR* hdf_obj_value (HDF *hdf, char **value);

NEOERR* hdf_set_value (HDF *hdf, char *name, char *value);
NEOERR* hdf_set_copy (HDF *hdf, char *dest, char *src);
NEOERR* hdf_set_buf (HDF *hdf, char *name, char *value);

NEOERR* hdf_read_file (HDF *hdf, char *path);

NEOERR* hdf_dump (HDF *hdf, char *prefix);

__END_DECLS

#endif /* __NEO_HDF_H_ */
