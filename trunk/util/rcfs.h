/*
 * Copyright 2001-2004 Brandon Long
 * All Rights Reserved.
 *
 * ClearSilver Templating System
 *
 * This code is made available under the terms of the ClearSilver License.
 * http://www.clearsilver.net/license.hdf
 *
 */

/*
 * revision-controlled file system (RCFS) with meta-info storage
 */
#ifndef __RCFS_H_
#define __RCFS_H_ 1

typedef struct _rcfs RCFS;

NEOERR * rcfs_init (RCFS **rcfs);
NEOERR * rcfs_destroy (RCFS **rcfs);

NEOERR * rcfs_load (const char *path, int version, char **data);
NEOERR * rcfs_save (const char *path, const char *data, const char *user, 
                    const char *log);
NEOERR * rcfs_lock (const char *path, int *lock);
void rcfs_unlock (int lock);
NEOERR * rcfs_meta_load (const char *path, HDF **meta);
NEOERR * rcfs_meta_save (const char *path, HDF *meta);
NEOERR * rcfs_listdir (const char *path, ULIST **list);
NEOERR * rcfs_link (const char *src_path, const char *dest_path);
NEOERR * rcfs_unlink (const char *path);

#endif /* __RCFS_H_ */
