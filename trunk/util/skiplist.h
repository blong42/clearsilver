

#ifndef _SKIPLIST_H_
#define _SKIPLIST_H_

/* 
 * Larger values of <root> means fewer levels and faster lookups,
 * but more variability in those lookup times (range limited from 2 to 4).
 *
 * <maxLevel> should be calculated from expected list size using (^ = power):
 *
 *     <root> ^ <maxLevel> == expected # of items 
 *
 * I've capped <maxLevel> at 20, which would be good for a minimum of
 * 1 million items on lists with <root> == 2.
 */
#define SKIP_MAXLEVEL 20

/* SKIP LIST TYPEDEFS */
typedef struct skipList *skipList;
typedef void (*skipFreeValue)(void *value, void *ctx);

NEOERR *skipNewList(skipList *skip, int threaded, int root, int maxLevel,
                     int flushLimit, skipFreeValue freeValue, void *ctx);
/*
 * Function:    skipNewList - create a skip list.
 * Description: Returns a new skip list.  If <threaded> is true, list is
 *              multi-thread safe.  <root> and <maxLevel> determine 
 *              performance and expected size (see discussion above).
 *              <flushLimit> is for threaded lists and determines the
 *              maximum number of deleted items to keep cached during
 *              concurrent searches.  Once the limit is reached, new 
 *              concurrent reads are blocked until all deleted items are 
 *              flushed.
 * Input:       threaded - true if list should be thread-safe.
 *              root - performance parameter (see above).
 *              maxLevel - performance parameter (see above).
 *              flushLimit - max deleted items to keep cached before
 *                forcing a flush.
 *              freeValue - callback made whenever a value is flushed.
 *              ctx - context to pass to <freeValue>.
 * Output:      None.
 * Return:      New skip list, NULL on error.
 * MT-Level:    Safe.
 */

void skipFreeList(skipList list);
/*
 * Function:    skipFreeList - free a skip list.
 * Description: Release all resources used by <list> including all key/value
 *              pairs.
 * Input:       list - list to free.
 * Output:      None.
 * Return:      None.
 * MT-Level:    Safe for unique <list>.
 */

void *skipNext(skipList list, UINT32 *pkey, void **plock);
/*
 * Function:    skipNext - find next item.
 * Description: Searches in list <list> for item with key next larger
 *              that the one in <pkey>, and returns its value if 
 *              found, or NULL if not.  If <plock> is non-NULL, then
 *              the lock returned in <plock> will be associated with
 *              the returned value.  Until this lock is passed to
 *              skipRelease(), the value will not be freed with the
 *              freeValue callback (see skipNewList()).
 * Input:       list - list to search in.
 *              pkey - pointer to previous key (0 to start).
 *              plock - place for value lock (or NULL).
 * Output:      pkey - set to new key.
 *              plock - set to value lock.
 * Return:      Value associated with new <pkey>, or NULL last item.
 * MT-Level:    Safe if <list> thread-safe.
 */

void *skipSearch(skipList list, UINT32 key, void **plock);
/*
 * Function:    skipSearch - search a skip list.
 * Description: Searches for <key> in <list>, and returns value if 
 *              found, or NULL if not.  If <plock> is non-NULL, then
 *              the lock returned in <plock> will be associated with
 *              the returned value.  Until this lock is passed to
 *              skipRelease(), the value will not be freed with the
 *              freeValue callback (see skipNewList()).
 * Input:       list - list to search in.
 *              key - key to look for.
 *              plock - place for value lock (or NULL).
 * Output:      plock - set to value lock.
 * Return:      Value associated with <key>, or NULL if <key> not found.
 * MT-Level:    Safe if <list> thread-safe.
 */

void skipRelease(skipList list, void *lock);
/*
 * Function:    skipRelease - release lock on value.
 * Description: Releases the lock on the value associated with <lock>.  Once
 *              the lock is released, the freeValue callback can be called
 *              and the item freed (see skipNewList()).
 * Input:       list - list containing value to release.
 *              lock - lock to release.
 * Output:      None.
 * Return:      None.
 * MT-Level:    Safe if <list> thread-safe.
 */

NEOERR *skipInsert(skipList list, UINT32 key, void *value, int allowUpdate);
/*
 * Function:    skipInsert - insert an item.
 * Description: Inserts the <key>/<value> pair into the <list>.
 *              Key values 0 and -1 are reserved (and illegal).
 *              If key is already in list, and <allowUpdate> is true, 
 *              value is updated, otherwise SKIPERR_EXISTS is returned.
 * Input:       list - list to add pair to.
 *              key - key identifying <value>.
 *              value - value to store (may NOT be NULL)
 * Output:      None.
 * Return:      1 on insert, 0 on update, or <0 on error:
 *                SKIPERR_EXISTS
 *                DBERR_MEMORY
 * MT-Level:    Safe if <list> thread-safe.
 */

void skipDelete(skipList list, UINT32 key);
/*
 * Function:    skipDelete - delete an item.
 * Description: Delete the item associated with <key> from <list>.
 * Input:       list - list to delete item from.
 *              key - key identifying value to delete.
 * Output:      None.
 * Return:      None.
 * MT-Level:    Safe if <list> thread-safe.
 */

#endif                                                       /* _SKIPLIST_H_ */
