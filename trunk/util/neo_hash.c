/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2003 by Brandon Long
 */

#include "cs_config.h"

#include <stdlib.h>
#include <string.h>

#include "neo_misc.h"
#include "neo_err.h"
#include "neo_hash.h"

static NEOERR *hash_resize(HASH *hash);
static HASHNODE **hash_lookup_node (HASH *hash, void *key, UINT32 *hashv);

NEOERR *hash_init (HASH **hash, HASH_FUNC hash_func, COMP_FUNC comp_func)
{
  HASH *my_hash = NULL;

  my_hash = (HASH *) calloc(1, sizeof(HASH));
  if (my_hash == NULL)
    return nerr_raise(NERR_NOMEM, "Unable to allocate memory for HASH");

  my_hash->size = 256;
  my_hash->num = 0;
  my_hash->hash_func = hash_func;
  my_hash->comp_func = comp_func;

  my_hash->nodes = (HASHNODE **) calloc (my_hash->size, sizeof(HASHNODE *));
  if (my_hash->nodes == NULL)
  {
    free(my_hash);
    return nerr_raise(NERR_NOMEM, "Unable to allocate memory for HASHNODES");
  }

  *hash = my_hash;

  return STATUS_OK;
}

void hash_destroy (HASH **hash)
{
  HASH *my_hash;
  HASHNODE *node, *next;
  int x;

  if (hash == NULL || *hash == NULL)
    return;

  my_hash = *hash;

  for (x = 0; x < my_hash->size; x++)
  {
    node = my_hash->nodes[x];
    while (node)
    {
      next = node->next;
      free(node);
      node = next;
    }
  }
  free(my_hash->nodes);
  my_hash->nodes = NULL;
  free(my_hash);
  *hash = NULL;
}

NEOERR *hash_insert(HASH *hash, void *key, void *value)
{
  UINT32 hashv;
  HASHNODE **node;

  node = hash_lookup_node(hash, key, &hashv);

  if (*node)
  {
    (*node)->value = value;
  }
  else
  {
    *node = (HASHNODE *) malloc(sizeof(HASHNODE));
    if (node == NULL)
      return nerr_raise(NERR_NOMEM, "Unable to allocate HASHNODE");

    (*node)->hashv = hashv;
    (*node)->key = key;
    (*node)->value = value;
    (*node)->next = NULL;
  }
  hash->num++;

  return hash_resize(hash);
}

void *hash_lookup(HASH *hash, void *key)
{
  HASHNODE *node;

  node = *hash_lookup_node(hash, key, NULL);

  return (node) ? node->value : NULL;
}

void *hash_remove_BROKEN(HASH *hash, void *key)
{
  HASHNODE **node, *remove;
  void *value = NULL;

  node = hash_lookup_node(hash, key, NULL);
  if (*node)
  {
    remove = *node;
    *node = remove->next;
    value = remove->value;
    free(remove);
    hash->num--;
  }
  return value;
}

int hash_has_key(HASH *hash, void *key)
{
  HASHNODE *node;

  node = *hash_lookup_node(hash, key, NULL);

  if (node) return 1;
  return 0;
}

void *hash_next(HASH *hash, void **key)
{
  HASHNODE **node = 0;
  UINT32 hashv, bucket;

  if (*key)
  {
    node = hash_lookup_node(hash, key, NULL);

    if (*node)
    {
      bucket = (*node)->hashv & (hash->size - 1);
    }
    else
    {
      hashv = hash->hash_func(*key);
      bucket = hashv & (hash->size - 1);
    }
  }
  else
  {
    bucket = 0;
  }

  if (*node)
  {
    if ((*node)->next)
    {
      *key = (*node)->next->key;
      return (*node)->next->value;
    }
    bucket++;
  }

  while (bucket < hash->size)
  {
    if (hash->nodes[bucket])
    {
      *key = hash->nodes[bucket]->key;
      return hash->nodes[bucket]->value;
    }
    bucket++;
  }

  return NULL;
}

static HASHNODE **hash_lookup_node (HASH *hash, void *key, UINT32 *o_hashv)
{
  UINT32 hashv, bucket;
  HASHNODE **node;

  hashv = hash->hash_func(key);
  if (o_hashv) *o_hashv = hashv;
  bucket = hashv & (hash->size - 1);
  /* ne_warn("Lookup %s %d %d", key, hashv, bucket); */

  node = &(hash->nodes[bucket]);

  if (hash->comp_func)
  {
    while (*node && !(hash->comp_func((*node)->key, key)))
      node = &(*node)->next;
  }
  else
  {
    /* No comp_func means we're doing pointer comparisons */
    while (*node && (*node)->key != key)
      node = &(*node)->next;
  }

  /* ne_warn("Node %x", node); */
  return node;
}

/* Ok, we're doing some weirdness here... */
static NEOERR *hash_resize(HASH *hash)
{
  HASHNODE **new_nodes;
  HASHNODE *entry, *prev;
  int x, next_bucket;
  int orig_size = hash->size;
  UINT32 hash_mask;

  if (hash->size > hash->num)
    return STATUS_OK;

  /* We always double in size */
  new_nodes = (HASHNODE **) realloc (hash->nodes, (hash->size*2) * sizeof(HASHNODE));
  if (new_nodes == NULL)
    return nerr_raise(NERR_NOMEM, "Unable to allocate memory to resize HASH");

  hash->nodes = new_nodes;
  orig_size = hash->size;
  hash->size = hash->size*2;

  /* Initialize new parts */
  for (x = orig_size; x < hash->size; x++)
  {
    hash->nodes[x] = NULL;
  }

  hash_mask = hash->size - 1;

  for (x = 0; x < orig_size; x++)
  {
    prev = NULL;
    next_bucket = x + orig_size;
    for (entry = hash->nodes[x]; 
	 entry; 
	 entry = prev ? prev->next : hash->nodes[x]) 
    {
      if ((entry->hashv & hash_mask) != x)
      {
	if (prev)
	{
	  prev->next = entry->next;
	}
	else
	{
	  hash->nodes[x] = entry->next;
	}
	entry->next = hash->nodes[next_bucket];
	hash->nodes[next_bucket] = entry;
      }
      else
      {
	prev = entry;
      }
    }
  }
  
  return STATUS_OK;
}

int hash_str_comp(const void *a, const void *b)
{
  return !strcmp((const char *)a, (const char *)b);
}

UINT32 hash_str_hash(const void *a)
{
  return ne_crc((char *)a, strlen((const char *)a));
}
