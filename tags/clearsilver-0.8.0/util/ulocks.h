/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef _ULOCKS_H_
#define _ULOCKS_H_


NEOERR *fCreate(int *plock, char *file);
/*
 * Function:    fCreate - create a file lock.
 * Description: Creates a file lock on named file <file>.  The lock is
 *              returned in <plock>.
 * Input:       plock - place for lock.
 *              file - path of file to use as lock.
 * Output:      plock - set to lock identifier.
 * Return:      STATUS_OK on success
 *              NERR_IO on failure
 * MT-Level:    Safe.
 */

NEOERR *fFind(int *plock, char *file);
/*
 * Function:    fFind - find a file lock.
 * Description: Find a file identified by the path <file>, and returns a 
 *              lock identifier for it in <plock>.  If the file doesn't
 *              exist, returns true.
 * Input:       plock - place for lock.
 *              file - path of file to use as lock.
 * Output:      plock - set to lock identifier.
 * Return:      STATUS_OK if found
 *              NERR_IO on failure
 * MT-Level:    Safe.
 */

void fDestroy(int lock);
/*
 * Function:    fDestroy - destroy a lock.
 * Description: Destroys the lock <lock> that was created by fCreate()
 *              or fFind().
 * Input:       lock - Lock to destroy.
 * Output:      None.
 * Return:      None.
 * MT-Level:    Safe for unique <lock>.
 */

NEOERR *fLock(int lock);
/*
 * Function:    fLock - acquire file lock.
 * Description: Acquires the lock identified by <lock>.  This call
 *              blocks until the lock is acquired.
 * Input:       lock - Lock to acquire.
 * Output:      None.
 * Return:      STATUS_OK on success
 *              NERR_LOCK on failure
 * MT-Level:    Safe.
 */

void fUnlock(int lock);
/*
 * Function:    fUnlock - release file lock.
 * Description: Releases the lock identified by <lock>.
 * Input:       lock - Lock to release.
 * Output:      None.
 * Return:      None.
 * MT-Level:    Safe.
 */

#ifdef HAVE_PTHREAD

#include <pthread.h>


NEOERR *mCreate(pthread_mutex_t *mutex);
/*
 * Function:    mCreate - initialize a mutex.
 * Description: Initializes the mutex <mutex>.
 * Input:       mutex - mutex to initialize.
 * Output:      None.
 * Return:      STATUS_OK on success
 *              NERR_LOCK on failure
 * MT-Level:    Safe for unique <mutex>.
 */

void mDestroy(pthread_mutex_t *mutex);
/*
 * Function:    mDestroy - destroy a mutex.
 * Description: Destroys the mutex <mutex> that was initialized by mCreate().
 * Input:       mutex - mutex to destroy.
 * Output:      None.
 * Return:      None.
 * MT-Level:    Safe for unique <mutex>.
 */

NEOERR *mLock(pthread_mutex_t *mutex);
/*
 * Function:    mLock - lock a mutex.
 * Description: Locks the mutex <mutex>.  This call blocks until the mutex
 *              is acquired.
 * Input:       mutex - mutex to lock.
 * Output:      None.
 * Return:      STATUS_OK on success
 *              NERR_LOCK on failure
 * MT-Level:    Safe.
 */

NEOERR *mUnlock(pthread_mutex_t *mutex);
/*
 * Function:    mUnlock - unlock a mutex.
 * Description: Unlocks the mutex <mutex>.
 * Input:       mutex - mutex to unlock.
 * Output:      None.
 * Return:      STATUS_OK on success
 *              NERR_LOCK on failure
 * MT-Level:    Safe.
 */

NEOERR *cCreate(pthread_cond_t *cond);
/*
 * Function:    cCreate - initialize a condition variable.
 * Description: Initializes the condition variable <cond>.
 * Input:       cond - condition variable to initialize.
 * Output:      None.
 * Return:      STATUS_OK on success
 *              NERR_LOCK on failure
 * MT-Level:    Safe for unique <cond>.
 */

void cDestroy(pthread_cond_t *cond);
/*
 * Function:    cDestroy - destroy a condition variable.
 * Description: Destroys the condition variable <cond> that was 
 *              initialized by cCreate().
 * Input:       cond - condition variable to destroy.
 * Output:      None.
 * Return:      None.
 * MT-Level:    Safe for unique <cond>.
 */

NEOERR *cWait(pthread_cond_t *cond, pthread_mutex_t *mutex);
/*
 * Function:    cWait - wait a condition variable signal.
 * Description: Waits for a signal on condition variable <cond>.
 *              The mutex <mutex> must be locked by the thread.
 * Input:       cond - condition variable to wait on.
 *              mutex - locked mutex to protect <cond>.
 * Output:      None.
 * Return:      STATUS_OK on success
 *              NERR_LOCK on failure
 * MT-Level:    Safe.
 */

NEOERR *cBroadcast(pthread_cond_t *cond);
/*
 * Function:    cBroadcast - broadcast signal to all waiting threads.
 * Description: Broadcasts a signal to all threads waiting on condition
 *              variable <cond>.
 * Input:       cond - condition variable to broadcast on.
 * Output:      None.
 * Return:      STATUS_OK on success
 *              NERR_LOCK on failure
 * MT-Level:    Safe.
 */

NEOERR *cSignal(pthread_cond_t *cond);
/*
 * Function:    cSignal - send signal to one waiting thread.
 * Description: Sends a signal to one thread waiting on condition
 *              variable <cond>.
 * Input:       cond - condition variable to send signal on.
 * Output:      None.
 * Return:      STATUS_OK on success
 *              NERR_LOCK on failure
 * MT-Level:    Safe.
 */

#endif /* HAVE_PTHREAD */

#endif                                                         /* _ULOCKS_H_ */
