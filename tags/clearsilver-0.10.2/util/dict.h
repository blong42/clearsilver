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


#ifndef __DICT_H_
#define __DICT_H_

__BEGIN_DECLS

typedef struct _dictCtx *dictCtx;
typedef BOOL (*dictCleanupFunc)(char *id, void *value, void *rock);
typedef void (*dictFreeValueFunc)(void *value, void *rock);

NEOERR *dictCreate(dictCtx *dict, BOOL threaded, UINT32 root, UINT32 maxLevel, 
    UINT32 flushLimit, BOOL useCase, 
    dictFreeValueFunc freeValue, void *freeRock);
/*
 * Function:    dictCreate - create new dictionary.
 * Description: Returns a dictionary.  If <threaded> is true, list is
 *              multi-thread safe.  <root>, <maxLevel>, and <flushLimit>
 *              act as for skipNewList() (see skiplist.h)
 * Input:       threaded - true if list should be thread-safe.
 *              root - performance parameter (see above).
 *              maxLevel - performance parameter (see above).
 *              flushLimit - max deleted items to keep cached before
 *                forcing a flush.
 *              useCase - true to be case sensitive in identifiers
 *              freeValue - callback when freeing a value
 *              freeRock - context for freeValue callback
 * Output:      None.
 * Return:      New dictionary, NULL on error.
 * MT-Level:    Safe.
 */

void dictDestroy(dictCtx dict);
/*
 * Function:    dictDestroy - destroy dictionary.
 * Description: Release all resources used by <dict>.
 * Input:       dict - dictionary to destroy
 * Output:      None.
 * Return:      None.
 * MT-Level:    Safe for unique <dict>.
 */

BOOL dictRemove(dictCtx dict, const char *id);
/*
 * Function:    dictRemove - remove item from dictionary.
 * Description: Removes item identified by <id> from <dict>.
 * Input:       dict - dictionary to search in.
 *              id - identifier of item to remove.
 * Output:      None.
 * Return:      true if item found, false if not.
 * MT-Level:    Safe if <dict> thread-safe.
 */

void *dictSearch(dictCtx dict, const char *id, void **plock);
/*
 * Function:    dictSearch - search for value in dictionary.
 * Description: Searches for <id> in <dict>, and returns value if 
 *              found, or NULL if not.  If <plock> is non-NULL, then
 *              the lock returned in <plock> will be associated with
 *              the returned value.  Until this lock is passed to
 *              dictReleaseLock(), the value will not be passed to the
 *              dictCleanupFunc callback (see dictCleanup()).
 * Input:       dict - dictionary to search in.
 *              id - identifier of item to find.
 *              plock - place for value lock (or NULL).
 * Output:      plock - set to value lock.
 * Return:      Value associated with <id>, or NULL if <id> not found.
 * MT-Level:    Safe if <dict> thread-safe.
 */

void *dictNext(dictCtx dict, char **id, void **plock);
/*
 * Function:    dictNext - search for next value in dictionary.
 * Description: Can be used to iterate through values in the dictionary.
 *              The order is the order of the hash of the ids, which
 *              isn't usefully externally.  Will return the value if 
 *              found, or NULL if not.  If <plock> is non-NULL, then
 *              the lock returned in <plock> will be associated with
 *              the returned value.  Until this lock is passed to
 *              dictReleaseLock(), the value will not be passed to the
 *              dictCleanupFunc callback (see dictCleanup()).
 * Input:       dict - dictionary to iterate over.
 *              id - pointer to identifier of last item found, or
 *                   pointer to NULL to retrieve first.
 *              plock - place for value lock (or NULL).
 * Output:      plock - set to value lock.
 *              id - pointer to id of found value
 * Return:      Value associated with <id>, or NULL if <id> not found.
 * MT-Level:    Safe if <dict> thread-safe.
 */

void dictReleaseLock(dictCtx dict, void *lock);
/*
 * Function:    dictReleaseLock - release lock on value.
 * Description: Releases the lock on the value associated with <lock>.  Once
 *              the lock is released, the dictCleanupFunc callback can
 *              be called for the value (see dictCleanup()).
 * Input:       dict - dictionary containing value to release.
 *              lock - lock to release.
 * Output:      None.
 * Return:      None.
 * MT-Level:    Safe if <dict> thread-safe.
 */

NEOERR *dictSetValue(dictCtx dict, const char *id, void *value);
/*
 * Function:    dictSetValue - set/reset an items value.
 * Description: Updates the <id>/<value> pair into <dict>.
 *              If <id> is not in <dict>, it is created.
 * Input:       dict - dictionary to add pair to.
 *              id - identifier to insert/update
 *              value - value to store (may NOT be NULL)
 * Output:      None.
 * Return:      true if inserted/updated, false if error
 * MT-Level:    Safe if <dict> thread-safe.
 */

typedef NEOERR *(*dictNewValueCB)(const char *id, void *rock, void **new_val);
typedef NEOERR *(*dictUpdateValueCB)(const char *id, void *value, void *rock);

NEOERR *dictModifyValue(dictCtx dict, const char *id, dictNewValueCB new_cb, 
                     dictUpdateValueCB update, void *rock);
/*
 * Function:    dictModifyValue - create/modify an item.
 * Description: Finds <id>'s value and calls <update>.  If <id> is
 *              not in <dict>, calls <new> to obtain a new value.
 * Input:       dict - dictionary to add pair to.
 *              id - identifier of value
 *              new - function to call to create new value (may be NULL)
 *              update - function to call to modify value (if NULL, the old
 *                 value is freed, and <new> is used)
 *              rock - context to pass to <new> or <update>.
 * Output:      None.
 * Return:      true if inserted/updated, false if error
 * MT-Level:    Safe if <dict> thread-safe.
 */

void dictCleanup(dictCtx dict, dictCleanupFunc cleanup, void *rock);
/*
 * Function:    dictCleanup - cleanup dictionary
 * Description: Calls <cleanup> for every item in <dict>.  If <cleanup>
 *              returns true, then item is removed from <dict>.
 * Input:       dict - dictionary to cleanup
 *              cleanup - cleanup callback
 *              rock - to pass to <cleanup>
 * Output:      None.
 * Return:      None.
 * MT-Level:    Safe if <dict> thread-safe.
 */

__END_DECLS

#endif                                                          /* __DICT_H_ */
