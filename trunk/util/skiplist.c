/* 
 *
 * Thread-safe Skiplist Using Integer Identifiers
 * Copyright 1998-2000 Scott Shambarger (scott@shambarger.net)
 *
 * This software is open source. Permission to use, copy, modify, and
 * distribute this software for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies.  No
 * warranty of any kind is expressed or implied.  Use at your own risk.
 *
 * 1/14/2001 blong
 *   Made it use neo errs... probably need to check locking functions
 *   for error returns...
 *
 */

#include "cs_config.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "neo_misc.h"
#include "neo_err.h"
#include "skiplist.h"
#include "ulocks.h"

typedef struct skipItem *skipItem;

/* structure is sized on allocation based on its level */
struct skipItem {
  UINT32 locks;                                   /* count of locks on value */
  UINT32 key;                                                  /* item's key */
  void *value;                                               /* item's value */
  INT32 level;                                                 /* item level */
  skipItem next[1];                                   /* array of next items */
};

#define SIZEOFITEM(max) (sizeof(struct skipItem) + \
                         ((max+1) * sizeof(skipItem)))

struct skipList_struct {
  INT32 topLevel;                           /* current max level in any item */
  INT32 levelHint;                          /* hint at level to start search */
  skipItem header;                           /* header item (has all levels) */
  skipItem tail;                               /* tail item (has all levels) */

  /* elements to handle cached deleted items */
  skipItem deleted; /* cached deleted items (linked by level+1 next entries) */
  UINT32 cached;                            /* number of cached deleted items */

  int flushing;             /* TRUE if thread waiting to flush cached items */
  UINT32 readers;                                /* number of current readers */
  int block;                                 /* TRUE if readers should wait */

  pthread_mutex_t read;                     /* readers count/cond wait mutex */
  pthread_mutex_t write;                                     /* writer mutex */
  pthread_cond_t resume;             /* condition to wait on to resume reads */
  pthread_cond_t flush;                    /* condition to wait on for flush */

  /* list constants */
  int threaded;                     /* TRUE if list needs to be thread safe */
  UINT32 flushLimit;      /* max number of cached deleted items before flush */
  INT32 maxLevel;                                /* max level list can reach */
  double randLimit;                       /* min random value to jump levels */
  skipFreeValue freeValue;                            /* free value callback */
  void *freeValueCtx;             /* context to pass to <freeValue> callback */
};

static void readLock(skipList list) {

  mLock(&list->read);

  if(list->block)
    cWait(&list->resume, &list->read);

  list->readers++;

  mUnlock(&list->read);

  return;
}

static void readUnlock(skipList list, skipItem x, void **plock) {

  int startFlush = FALSE;

  if(list->threaded)
    mLock(&list->read);

  if(plock) {
    x->locks++;
    *plock = x;
  }

  if(! list->threaded)
    return;

  list->readers--;

  if((list->readers == 0) && list->block)
    startFlush = TRUE;

  mUnlock(&list->read);

  if(startFlush)
    cSignal(&list->flush);

  return;
}

static void readBlock(skipList list) {

  mLock(&list->read);

  list->block = TRUE;

  if(list->readers)
    cWait(&list->flush, &list->read);    /* wait until reader locks released */

  return;
}

static void readUnblock(skipList list) {

  list->block = FALSE;

  mUnlock(&list->read);

  cBroadcast(&list->resume);

  return;
}


static void writeLock(skipList list) {

  mLock(&list->write);

  return;
}

static void writeUnlock(skipList list) {

  mUnlock(&list->write);

  return;
}

static NEOERR *skipAllocItem(skipItem *item, UINT32 level, UINT32 key, 
    void *value) 
{

  if(! (*item = malloc(SIZEOFITEM(level))))
    return nerr_raise(NERR_NOMEM, "Unable to allocate space for skipItem");

  /* init new item */
  (*item)->locks = 0;
  (*item)->key = key;
  (*item)->value = value;
  (*item)->level = level;

  return STATUS_OK;
}

static void skipFreeItem(skipList list, skipItem item) {

  if(list->freeValue)
    list->freeValue(item->value, list->freeValueCtx);          /* free value */

  free(item);                                                /* release item */

  return;
}

static void skipFlushDeleted(skipList list, int force) {

  skipItem x, y, next;

  x = list->deleted;
  y = x->next[x->level + 1];

  while(y != list->tail) {

    next = y->next[y->level + 1];

    if(force || (! y->locks)) {           /* check if value currently locked */

      x->next[x->level + 1] = next;         /* set previous item's next link */
      skipFreeItem(list, y);                                    /* free item */

      list->cached--;                                 /* update cached count */
    }
    else {
      x = y;                             /* make this item the previous item */
    }

    y = next;                                        /* advance to next item */
  }
  
  return;
}

static void skipWriteUnlock(skipList list) {

  int flush;

  if(! list->threaded)
    return;

  if((list->cached > list->flushLimit) && (! list->flushing)) {
    list->flushing = TRUE;
    flush = TRUE;
  }
  else {
    flush = FALSE;
  }

  writeUnlock(list);                      /* let any pending writes complete */
  readUnlock(list, NULL, NULL);                         /* no longer reading */

  if(flush) {
                                        /* we are now flushing deleted items */

    readBlock(list);                               /* acquire all read locks */

                              /* at this point no readers/writers are active */

    skipFlushDeleted(list, FALSE);                    /* flush deleted items */

    list->flushing = FALSE;                                 /* done flushing */

    readUnblock(list);                              /* let everyone continue */
  }

  return;
}

static skipItem skipFind(skipList list, UINT32 key) {

  skipItem x, y = NULL;
  INT32 i;

  if(list->threaded)
    readLock(list);

  x = list->header;                            /* header contains all levels */

  for(i = list->levelHint;      /* loop from levelHint level down to level 0 */
      i >= 0;
      i--) {

    y = x->next[i];                            /* get next item at new level */

    while(y->key < key) {       /* if y has a smaller key, try the next item */
      x = y;                                  /* save x in case we overshoot */
      y = x->next[i];                                       /* get next item */
    }
  }

  return y;
}

void *skipSearch(skipList list, UINT32 key, void **plock) {

  skipItem y;
  void *value;

  y = skipFind(list, key);                                      /* find item */

  if(y->key == key) {                     /* y has our key, or it isn't here */
    value = y->value;
  }
  else {                              /* didn't find item, don't allow locks */
    value = NULL;
    plock = NULL;
  }

  readUnlock(list, y, plock);

  return value;
}

void *skipNext(skipList list, UINT32 *pkey, void **plock) {

  skipItem y;
  void *value;

  y = skipFind(list, *pkey);                                    /* find item */

  if((y->key == *pkey) && (y != list->tail))      /* skip to next if found y */
    y = y->next[0];

  if(y != list->tail) {                   /* reset key to next, return value */
    *pkey = y->key;
    value = y->value;
  }
  else {                                  /* no next item, don't allow locks */
    value = NULL;
    plock = NULL;
  }

  readUnlock(list, y, plock);

  return value;
}

void skipRelease(skipList list, void *lock) {

  skipItem x;

  mLock(&list->read);

  x = lock;
  x->locks--;

  mUnlock(&list->read);

  return;
}

/* list is write locked */
static NEOERR *skipNewItem(skipList list, skipItem *item, UINT32 key, 
    void *value) 
{

  INT32 level = 0;

  while((drand48() < list->randLimit) && (level < list->maxLevel))
    level++;

  if(level > list->topLevel) {

    if(list->topLevel < list->maxLevel)
      list->topLevel++;

    level = list->topLevel;
  }

  return skipAllocItem(item, level, key, value);
}

/* list is write locked */
static void skipDeleteItem(skipList list, skipItem item) {

  if(list->threaded) {
    item->next[item->level + 1] = list->deleted->next[1];
    list->cached++;
    list->deleted->next[1] = item;
  }
  else {
    skipFreeItem(list, item);
  }

  return;
}

NEOERR *skipNewList(skipList *skip, int threaded, int root, int maxLevel,
                     int flushLimit, skipFreeValue freeValue, void *ctx) 
{
  NEOERR *err;
  skipList list;
  UINT32 i;

  *skip = NULL;
  if(! (list = calloc(1, sizeof(struct skipList_struct))))
    return nerr_raise(NERR_NOMEM, "Unable to allocate memore for skiplist");

  if (maxLevel == 0)
    return nerr_raise(NERR_ASSERT, "maxLevel must be greater than 0");

  if(maxLevel >= SKIP_MAXLEVEL)                              /* check limits */
    maxLevel = SKIP_MAXLEVEL-1;

  if(root > 4)
    root = 4;
  else if(root < 2)
    root = 2;

  list->maxLevel = maxLevel;                          /* init list constants */
  list->randLimit = 1.0 / (double)root;
  list->threaded = threaded;
  list->freeValue = freeValue;
  list->freeValueCtx = ctx;

  do {
    if(threaded) {

      list->flushLimit = flushLimit;

      err = mCreate(&list->read);
      if (err != STATUS_OK) break;

      err = mCreate(&list->write);
      if (err != STATUS_OK) break;

      err = cCreate(&list->resume);
      if (err != STATUS_OK) break;

      err = cCreate(&list->flush);
      if (err != STATUS_OK) break;
    }

    err = skipAllocItem(&(list->header), list->maxLevel, 0, NULL);
    if (err != STATUS_OK) break;
    err = skipAllocItem(&(list->tail), list->maxLevel, (UINT32)-1, NULL);
    if (err != STATUS_OK) break;
    err = skipAllocItem(&(list->deleted), 0, 0, NULL);
    if (err != STATUS_OK) break;

    for(i = 0;                                       /* init header and tail */
        i <= list->maxLevel;
        i++) {
      list->tail->next[i] = NULL;
      list->header->next[i] = list->tail;
    }

    list->deleted->next[1] = list->tail;

    *skip = list;
    return STATUS_OK;                                     /* return new list */

  } while(FALSE);

  if(list->header)                              /* failed to make list, bail */
    free(list->header);
  free(list);

  return nerr_pass(err);
}

/* list considered locked */
static void skipFreeAllItems(skipList list) {

  UINT32 i;
  skipItem x, y;

  x = list->header->next[0];

  while(x != list->tail) {
    y = x->next[0];                    /* get next item from level 0 pointer */
    skipFreeItem(list, x);                                   /* release item */
    x = y;
  }
                                                    /* clear header pointers */
  for(i = 0;
      i <= list->maxLevel;
      i++)
    list->header->next[i] = list->tail;

  return;
}

void skipFreeList(skipList list) {

  skipFlushDeleted(list, TRUE);                       /* flush deleted items */

  skipFreeAllItems(list);                                 /* free list items */
  
  if(list->threaded) {
    cDestroy(&list->flush);
    cDestroy(&list->resume);
    mDestroy(&list->write);
    mDestroy(&list->read);
  }

  free(list->tail);                                             /* free list */
  free(list->header);
  free(list->deleted);
  free(list);

  return;
}

/* <list> is locked, <x> is at least level <level>, and <x>->key < <key> */
static skipItem skipClosest(skipItem x, UINT32 key, UINT32 level) {

  skipItem y;

  y = x->next[level];                         /* get next item at this level */
  
  while(y->key < key) {       /* ensure that we have the item before the key */
    x = y;
    y = x->next[level];
  }

  return x;
}

static skipItem skipLock(skipList list, UINT32 key, skipItem *save, INT32 top) {

  INT32 i;
  skipItem x, y;

  if(list->threaded)
    readLock(list);

  x = list->header;                            /* header contains all levels */

  for(i = top;                        /* loop from top level down to level 0 */
      i >= 0;
      i--) {

    y = x->next[i];                           /* get next item at this level */

    while(y->key < key) {       /* if y has a smaller key, try the next item */
      x = y;                                  /* save x in case we overshoot */
      y = x->next[i];                                       /* get next item */
    }

    save[i] = x;                  /* preserve item with next pointer in save */
  }

  if(list->threaded)
    writeLock(list);                                 /* lock list for update */

                               /* validate we have the closest previous item */
  return skipClosest(x, key, 0);
}

NEOERR *skipInsert(skipList list, UINT32 key, void *value, int allowUpdate) 
{
  NEOERR *err;
  INT32 i, level;
  skipItem save[SKIP_MAXLEVEL];
  skipItem x, y;

  if (value == 0)
    return nerr_raise(NERR_ASSERT, "value must be non-zero");
  if (key == 0 || key == (UINT32)-1)
    return nerr_raise(NERR_ASSERT, "key must not be 0 or -1");

  level = list->levelHint;

  x = skipLock(list, key, save, level);              /* quick search for key */

  y = x->next[0];

  if(y->key == key) {

    if(!allowUpdate)
    {
      skipWriteUnlock(list);
      return nerr_raise(NERR_DUPLICATE, "key %u exists in skiplist", key);
    }

    y->value = value;                       /* found the key, update value */
    skipWriteUnlock(list);
    return STATUS_OK;
  }

  err = skipNewItem(list, &y, key, value);
  if (err != STATUS_OK)
  {
    skipWriteUnlock(list);
    return nerr_pass(err);
  }

  for(i = level + 1;             /* is new item has more levels than <level> */
      i <= y->level;                                   /* if so fill in save */
      i++)
    save[i] = list->header;

  for(i = 0;                             /* populate pointers for all levels */
      i <= y->level;
      i++) {
    
    if(i)                       /* check that save is correct for each level */
      x = skipClosest(save[i], key, i);

    y->next[i] = x->next[i];            /* now insert the item at this level */
    x->next[i] = y;            /* (order here important for thread-safeness) */
  }

  while((list->levelHint < list->topLevel)               /* update levelHint */
        && (list->header->next[list->levelHint+1] != list->tail)) 
    list->levelHint++;

  skipWriteUnlock(list);

  return STATUS_OK;
}

void skipDelete(skipList list, UINT32 key) {

  INT32 i, level;
  skipItem save[SKIP_MAXLEVEL];
  skipItem x, y;

  assert(key && (key != (UINT32)-1));

  level = list->levelHint;

  x = skipLock(list, key, save, level);              /* quick search for key */

  y = x->next[0];

                        /* check that we found the key, and it isn't deleted */
  if((y->key != key) || (y->next[0]->key < key)) {
    skipWriteUnlock(list);
    return;
  }

  for(i = level + 1;           /* check if item has more levels than <level> */
      i <= y->level;                                   /* if so fill in save */
      i++)
    save[i] = list->header;

  for(i = y->level;
      i >= 0;
      i--) {

                                /* check that save is correct for each level */
    x = skipClosest(save[i], key, i);

    x->next[i] = y->next[i];                /* now remove item at this level */
    y->next[i] = x;          /* (order here is imported for thread-safeness) */
  }

  skipDeleteItem(list, y);                            /* put on deleted list */

  while((list->levelHint > 0)                            /* update levelHint */
        && (list->header->next[list->levelHint] == list->tail))
    list->levelHint--;

  skipWriteUnlock(list);

  return;
}



