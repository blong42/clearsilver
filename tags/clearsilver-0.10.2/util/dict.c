/* 
 *
 * Thread-safe Dictionary Using String Identifiers
 * Copyright 1998-2000 Scott Shambarger (scott@shambarger.net)
 *
 * This software is open source. Permission to use, copy, modify, and
 * distribute this software for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies.  No
 * warranty of any kind is expressed or implied.  Use at your own risk.
 *
 */

#include "cs_config.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "neo_misc.h"
#include "neo_err.h"
#include "dict.h"
#include "skiplist.h"
#include "ulocks.h"


typedef struct dictValue {

  void *value;                                        /* value to set/update */
  dictNewValueCB new;                  /* new value callback (value is NULL) */
  dictUpdateValueCB update;         /* update value callback (value is NULL) */
  void *rock;                                   /* rock to pass to callbacks */

} *dictValuePtr;

typedef struct dictItem {

  struct dictItem *next;                            /* pointer to next value */
  char *id;                                                     /* string id */
  void *value;                                                      /* value */

} *dictItemPtr;

typedef struct dictEntry {

  dictItemPtr first;                                  /* first item in entry */
  BOOL deleted;               /* TRUE if entry has been passed to skipDelete */

} *dictEntryPtr;

typedef UINT32 (*dictHashFunc)(const char *str);
typedef int (*dictCompFunc)(const char *s1, const char *s2);

struct _dictCtx {

  pthread_mutex_t mList;                                /* list update mutex */
  skipList list;                                                /* skip list */

  dictHashFunc hash;                                        /* hash function */
  dictCompFunc comp;                                  /* id compare function */
  BOOL useCase;

  BOOL threaded;                                         /* TRUE if threaded */
  dictFreeValueFunc freeValue;                        /* free value callback */
  void *freeRock;                                   /* context for freeValue */
};

#undef DO_DEBUG

#ifdef DO_DEBUG
#include <sched.h>
#define DICT_LOCK(dict) \
  do { if((dict)->threaded) { sched_yield(); \
   mLock(&(dict)->mList); } } while(0)
#define DICT_HASH_BITS 16
#else
#define DICT_LOCK(dict) \
  if((dict)->threaded) mLock(&(dict)->mList)
#define DICT_HASH_BITS 65536
#endif
#define DICT_UNLOCK(dict) \
  if((dict)->threaded) mUnlock(&(dict)->mList)

/* entry is locked, so item may be added */
static NEOERR *dictNewItem(dictCtx dict, dictEntryPtr entry,
    const char *id, dictValuePtr newval, dictItemPtr *item) 
{
  dictItemPtr my_item;

  if (item != NULL)
    *item = NULL;

  /* check if we can set a new value */
  if(! (newval->value || newval->new))
    return nerr_raise(NERR_ASSERT, "value or new are NULL");

  if(! (my_item = calloc(1, sizeof(struct dictItem))))
    return nerr_raise(NERR_NOMEM, "Unable to allocate new dictItem");

  if(! (my_item->id = strdup(id))) {
    free(my_item);
    return nerr_raise(NERR_NOMEM, "Unable to allocate new id for dictItem");
  }

  /* set new value */
  if(newval->value) {
    my_item->value = newval->value;
  }
  else {
    NEOERR *err = STATUS_OK;

    err = newval->new(id, newval->rock, &(my_item->value));
    if (err != STATUS_OK)
    {
      /* new item callback failed, cleanup */
      free(my_item->id);
      free(my_item);

      return nerr_pass(err);
    }
  }

  my_item->next = entry->first;
  entry->first = my_item;
  if (item != NULL)
    *item = my_item;

  return STATUS_OK;
}

static void dictFreeItem(dictCtx dict, dictItemPtr item) {

  if(dict->freeValue)
    dict->freeValue(item->value, dict->freeRock);
  free(item->id);
  free(item);

  return;
}

/* list locked, so safe to walk entry */
static dictItemPtr dictFindItem(dictCtx dict, dictEntryPtr entry, 
                                const char *id, BOOL unlink) {

  dictItemPtr *prev, item;

  prev = &entry->first;

  for(item = entry->first; item; item = item->next) {

    if(! dict->comp(item->id, id)) {

      if(unlink)
        *prev = item->next;

      return item;
    }

    prev = &item->next;
  }

  return NULL;
}

static NEOERR *dictUpdate(dictCtx dict, dictEntryPtr entry, const char *id, 
                       dictValuePtr newval, void *lock) {

  NEOERR *err = STATUS_OK;
  dictItemPtr item = NULL;
  void *newValue;

  /* check for entry (maybe not found...) */
  if(! entry)
    return nerr_raise(NERR_NOT_FOUND, "Entry is NULL");

  /* only use entry if not deleted */
  if(! entry->deleted) {

    /* find item */
    if((item = dictFindItem(dict, entry, id, FALSE))) {
      
      if(newval->value) {

        if(dict->freeValue)
          dict->freeValue(item->value, dict->freeRock);
        
        item->value = newval->value;
      }
      else if(newval->update) {

        /* track error (if update fails) */
	err = newval->update(id, item->value, newval->rock);
      }
      else if((err = newval->new(id, newval->rock, &newValue)) == STATUS_OK) {

        if(dict->freeValue)
          dict->freeValue(item->value, dict->freeRock);
        
        item->value = newValue;
      }
      else {
        /* new item failed (don't remove old), indicate that update failed */
        item = NULL;
      }
    }
    else {
      
      /* add new item to entry */
      err = dictNewItem(dict, entry, id, newval, &item);
    }
  }

  /* release entry lock */
  skipRelease(dict->list, lock);

  return nerr_pass(err);
}

static NEOERR *dictInsert(dictCtx dict, UINT32 hash, const char *id, 
                          dictValuePtr newval) {

  dictEntryPtr entry;
  void *lock;
  NEOERR *err = STATUS_OK;

  /* create new item and insert entry */
  entry = calloc(1, sizeof(struct dictEntry));
  if (entry == NULL)
    return nerr_raise(NERR_NOMEM, "Unable to allocate memory for dictEntry");
    
  /* create/insert item (or cleanup) */
  err = dictNewItem(dict, entry, id, newval, NULL);
  if (err != STATUS_OK) return nerr_pass(err);

  /* if we insert, we're done */
  if((err = skipInsert(dict->list, hash, entry, FALSE)) == STATUS_OK)
    return STATUS_OK;

  /* failed to insert, cleanup */
  if(dict->freeValue && ! newval->value)
    dict->freeValue(entry->first->value, dict->freeRock);
  free(entry->first->id);
  free(entry->first);
  free(entry);

  /* check err */
  if (!nerr_handle(&err, NERR_DUPLICATE))
    return nerr_pass(err);
  
  /* cool, someone already inserted the entry before we got the lock */
  entry = skipSearch(dict->list, hash, &lock);

  /* update entry as normal (handles entry not found) */
  return nerr_pass(dictUpdate(dict, entry, id, newval, lock));
}

static UINT32 dictHash(dictCtx dict, const char *id) {

  UINT32 hash;

  hash = dict->hash(id) % DICT_HASH_BITS;

  /* ensure hash is valid for skiplist (modify consistently if not) */
  if(! (hash && (hash != (UINT32)-1)))
    hash = 1;

  return hash;
}

static NEOERR *dictModify(dictCtx dict, const char *id, dictValuePtr newval) 
{
  NEOERR *err;
  UINT32 hash;
  dictEntryPtr entry;
  void *lock = NULL;

  hash = dictHash(dict, id);

  /* find entry in list */
  entry = skipSearch(dict->list, hash, &lock);
    
  DICT_LOCK(dict);

  if((err = dictUpdate(dict, entry, id, newval, lock)) != STATUS_OK) 
  {
    /* insert new entry */
    nerr_ignore(&err);
    err = dictInsert(dict, hash, id, newval);
  }

  DICT_UNLOCK(dict);
  
  return nerr_pass(err);
}

NEOERR *dictSetValue(dictCtx dict, const char *id, void *value) {

  struct dictValue newval;

  assert(value);

  newval.value = value;

  return dictModify(dict, id, &newval);
}

NEOERR *dictModifyValue(dictCtx dict, const char *id, dictNewValueCB new, 
                     dictUpdateValueCB update, void *rock) {

  struct dictValue newval;

  if(! (new || update))
    return FALSE;

  newval.value = NULL;
  newval.new = new;
  newval.update = update;
  newval.rock = rock;

  return dictModify(dict, id, &newval);
}

void dictReleaseLock(dictCtx dict, void *lock) {

  /* release entry */
  DICT_UNLOCK(dict);

  /* release skip entry */
  skipRelease(dict->list, lock);

  return;
}

void dictCleanup(dictCtx dict, dictCleanupFunc cleanup, void *rock) {

  dictItemPtr *prev, item, next;
  dictEntryPtr entry;
  UINT32 key = 0;
  void *lock;

  while((entry = skipNext(dict->list, &key, &lock))) {

    DICT_LOCK(dict);

    prev = &entry->first;
    
    for(item = entry->first; item; item = next) {
      
      next = item->next;
      
      if(cleanup(item->id, item->value, rock)) {
        
        /* remove item */
        *prev = item->next;
        dictFreeItem(dict, item);
      }
      else {
        /* update reference pointer */
        prev = &item->next;
      }
    }

    /* delete entry if last item removed */
    if(! entry->first) {

      entry->deleted = TRUE;

      skipDelete(dict->list, key);
    }
    
    dictReleaseLock(dict, lock);
  }

  return;
}

void *dictSearch(dictCtx dict, const char *id, void **plock) {

  dictEntryPtr entry;
  dictItemPtr item;
  UINT32 hash;
  void *lock;
  void *value;

  hash = dictHash(dict, id);

  /* find entry in list */
  if(! (entry = skipSearch(dict->list, hash, &lock)))
    return NULL;

  /* lock entry */
  DICT_LOCK(dict);

  /* find item */
  if((item = dictFindItem(dict, entry, id, FALSE))) {

    value = item->value;

    if(plock)
      *plock = lock;
    else
      dictReleaseLock(dict, lock);

    return value;
  }

  dictReleaseLock(dict, lock);

  return NULL;
}

void *dictNext (dictCtx dict, char **id, void **plock)
{
  dictEntryPtr entry;
  dictItemPtr item;
  UINT32 hash;
  void *lock;
  void *value;

  /* Handle the first one special case */
  if (*id == NULL)
  {
    hash = 0;
    /* find entry in list */
    if(! (entry = skipNext (dict->list, &hash, &lock)))
      return NULL;

    /* lock entry */
    DICT_LOCK(dict);

    /* Take first item in list */
    item = entry->first;

    if (item != NULL)
    {
      value = item->value;
      *id = item->id;

      if(plock)
	*plock = lock;
      else
	dictReleaseLock(dict, lock);

      return value;
    }

    dictReleaseLock(dict, lock);

    return NULL;
  }
  else
  {
    hash = dictHash(dict, *id);

    /* find entry in list */
    entry = skipSearch (dict->list, hash, &lock);

    if (entry == NULL)
    {
      entry = skipNext (dict->list, &hash, &lock);
      /* Not found, we're at the end of the dict */
      if (entry == NULL)
	return NULL;
    }

    /* lock entry */
    DICT_LOCK(dict);

    item = dictFindItem(dict, entry, *id, FALSE);
    if (item != NULL)
    {
      if (item->next != NULL)
      {
	item = item->next;
      }
      else
      {
	/* we have to move to the next skip entry */
	entry = skipNext (dict->list, &hash, &lock);
	/* Not found, we're at the end of the dict */
        item = entry?entry->first:NULL;
        
	if(! item) {
          dictReleaseLock(dict, lock);
	  return NULL;
        }

      }
      value = item->value;
      *id = item->id;

      if(plock)
	*plock = lock;
      else
	dictReleaseLock(dict, lock);

      return value;
    }

    dictReleaseLock(dict, lock);
  }

  return NULL;
}

BOOL dictRemove(dictCtx dict, const char *id) {

  dictEntryPtr entry;
  dictItemPtr item;
  UINT32 hash;
  void *lock;

  hash = dictHash(dict, id);

  /* find entry in list */
  if(! (entry = skipSearch(dict->list, hash, &lock)))
    return FALSE;

  /* lock entry */
  DICT_LOCK(dict);

  /* find/unlink/free item */
  if((item = dictFindItem(dict, entry, id, TRUE)))
    dictFreeItem(dict, item);

  dictReleaseLock(dict, lock);

  return item ? TRUE : FALSE;
}

/* called by skipList when safe to destroy entry */
static void dictDestroyEntry(void *value, void *ctx) {

  dictItemPtr item, next;
  dictEntryPtr entry;

  entry = value;

  for(item = entry->first; item; item = next) {

    next = item->next;
    dictFreeItem(ctx, item);
    item = next;
  }

  free(value);

  return;
}

NEOERR *dictCreate(dictCtx *rdict, BOOL threaded, UINT32 root, UINT32 maxLevel, 
    UINT32 flushLimit, BOOL useCase, dictFreeValueFunc freeValue, void *freeRock) 
{
  NEOERR *err;
  dictCtx dict;

  *rdict = NULL;

  do {

    if(! (dict = calloc(1, sizeof(struct _dictCtx))))
      return nerr_raise (NERR_NOMEM, "Unable to allocate memory for dictCtx");

    dict->useCase = useCase;
    dict->hash = python_string_hash;
    if(useCase) {
      dict->comp = strcmp;
    }
    else {
      /* dict->hash = uhashUpper; */
      dict->comp = strcasecmp;
    }

    dict->threaded = threaded;
    dict->freeValue = freeValue;
    dict->freeRock = freeRock;

    err = skipNewList(&(dict->list), threaded, root, maxLevel, 
	    flushLimit, dictDestroyEntry, dict);
    if (err != STATUS_OK) break;

    if (threaded)
    {
      err = mCreate(&(dict->mList));
      if (err != STATUS_OK) break;
    }

    *rdict = dict;
    return STATUS_OK;

  } while(FALSE);

  dictDestroy(dict);

  return nerr_pass(err);
}

void dictDestroy(dictCtx dict) {

  if(! dict)
    return;

  skipFreeList(dict->list);

  mDestroy(&dict->mList);

  free(dict);

  return;
}
