/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef __NEO_HASH_H_
#define __NEO_HASH_H_ 1

__BEGIN_DECLS

#include <stdlib.h>
#include "util/neo_misc.h"

typedef UINT32 (*HASH_FUNC)(const void *);
typedef int (*COMP_FUNC)(const void *, const void *);

typedef struct _HASHNODE
{
  void *key;
  void *value;
  UINT32 hashv;
  struct _HASHNODE *next;
} HASHNODE;

typedef struct _HASH
{
  UINT32 size;
  UINT32 num;

  HASHNODE **nodes;
  HASH_FUNC hash_func;
  COMP_FUNC comp_func;
} HASH;

NEOERR *hash_init (HASH **hash, HASH_FUNC hash_func, COMP_FUNC comp_func);
void hash_destroy (HASH **hash);
NEOERR *hash_insert(HASH *hash, void *key, void *value);
void *hash_lookup(HASH *hash, void *key);
int hash_has_key(HASH *hash, void *key);
void *hash_remove(HASH *hash, void *key);
void *hash_next(HASH *hash, void **key);

int hash_str_comp(const void *a, const void *b);
UINT32 hash_str_hash(const void *a);

__END_DECLS

#endif /* __NEO_HASH_H_ */
