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

#include "cs_config.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/neo_files.h"
#include "util/neo_hdf.h"
#include "util/ulocks.h"
#include "rcfs.h"

NEOERR * rcfs_meta_load (const char *path, HDF **meta)
{
  NEOERR *err;
  char fpath[_POSIX_PATH_MAX];
  HDF *m;

  snprintf (fpath, sizeof(fpath), "%s,log", path);

  err = hdf_init (&m);
  if (err) return nerr_pass (err);
  err = hdf_read_file (m, fpath);
  if (err)
  {
    hdf_destroy (&m);
    return nerr_pass (err);
  }
  *meta = m;
  return STATUS_OK;
}

static NEOERR * _meta_save (const char *path, HDF *meta)
{
  NEOERR *err;
  char ftmp[_POSIX_PATH_MAX];
  char fpath[_POSIX_PATH_MAX];

  snprintf (ftmp, sizeof(ftmp), "%s,log.tmp", path);
  snprintf (fpath, sizeof(fpath), "%s,log", path);

  err = hdf_write_file (meta, ftmp);
  if (err) return nerr_pass (err);
  if (rename (ftmp, fpath) == -1)
  {
    unlink (ftmp);
    return nerr_raise_errno (NERR_IO, "Unable to rename file %s", ftmp);
  }

  return STATUS_OK;
}

NEOERR * rcfs_meta_save (const char *path, HDF *meta)
{
  NEOERR *err;
  int lock;
  HDF *m;

  err = rcfs_lock (path, &lock);
  if (err) return nerr_pass (err);
  do
  {
    err = rcfs_meta_load (path, &m);
    if (err) break;
    err = hdf_copy (m, "Meta", meta);
    if (err) break;
    err = _meta_save (path, m);
  } while (0);

  rcfs_unlock (lock);
  return nerr_pass (err);
}

/* load a specified version of the file, version -1 is latest */
NEOERR * rcfs_load (const char *path, int version, char **data)
{
  NEOERR *err;
  char fpath[_POSIX_PATH_MAX];

  if (version == -1)
  {
    HDF *meta, *vers;
    int x;

    err = rcfs_meta_load (path, &meta);
    if (err) return nerr_pass (err);
    for (vers = hdf_get_child (meta, "Versions");
	vers;
	vers = hdf_obj_next (vers))
    {
      x = atoi (hdf_obj_name (vers));
      if (x > version) version = x;
    }
    hdf_destroy (&meta);
  }
  snprintf (fpath, sizeof (fpath), "%s,%d", path, version);
  err = ne_load_file (fpath, data);
  return nerr_pass (err);
}

NEOERR * rcfs_save (const char *path, const char *data, const char *user, 
                    const char *log)
{
  NEOERR *err;
  HDF *meta = NULL, *vers;
  char fpath[_POSIX_PATH_MAX];
  char buf[256];
  int version = 0;
  int fd;
  int lock;
  int x, l, w;

  err = rcfs_lock (path, &lock);
  if (err) return nerr_pass (err);
  do
  {
    err = rcfs_meta_load (path, &meta);
    if (err && nerr_handle (&err, NERR_NOT_FOUND))
    {
      /* new file! */
      err = hdf_init (&meta);
    }
    if (err) return nerr_pass (err);
    for (vers = hdf_get_child (meta, "Versions");
	vers;
	vers = hdf_obj_next (vers))
    {
      x = atoi (hdf_obj_name (vers));
      if (x > version) version = x;
    }

    /* new version */
    version++;
    snprintf (fpath, sizeof (fpath), "%s,%d", path, version);
    fd = open (fpath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (fd == -1)
    {
      err = nerr_raise_errno (NERR_IO, "Unable to create file %s", fpath);
      break;
    }
    l = strlen(data);
    w = write (fd, data, l);
    if (w != l)
    {
      err = nerr_raise_errno (NERR_IO, "Unable to write file %s", fpath);
      close (fd);
      break;
    }
    close (fd);
    snprintf (buf, sizeof(buf), "Versions.%d.Log", version);
    err = hdf_set_value (meta, buf, log);
    if (err) break;
    snprintf (buf, sizeof(buf), "Versions.%d.User", version);
    err = hdf_set_value (meta, buf, user);
    if (err) break;
    snprintf (buf, sizeof(buf), "Versions.%d.Date", version);
    err = hdf_set_int_value (meta, buf, ne_timef());
    if (err) break;
    err = _meta_save (path, meta);
  } while (0);

  rcfs_unlock (lock);
  hdf_destroy (&meta);
  return nerr_pass (err);
}

NEOERR * rcfs_lock (const char *path, int *lock)
{
  NEOERR *err;
  char fpath[_POSIX_PATH_MAX];

  snprintf (fpath, sizeof (fpath), "%s,lock", path);
  err = fCreate (lock, fpath);
  if (err) return nerr_pass (err);
  err = fLock (*lock);
  if (err) 
  {
    fDestroy (*lock);
    return nerr_pass (err);
  } 
  return STATUS_OK;
}

void rcfs_unlock (int lock)
{
  fUnlock (lock);
  fDestroy (lock);
}

NEOERR * rcfs_listdir (const char *path, ULIST **list)
{
  NEOERR *err;
  DIR *dp;
  ULIST *files;
  struct dirent *de;
  int l;
  char *f;

  *list = NULL;
  err = uListInit (&files, 10, 0);
  if (err) return nerr_pass (err);
  dp = opendir(path);
  if (dp == NULL)
  {
    uListDestroy(&files, ULIST_FREE);
    if (errno == ENOENT)
      return nerr_raise (NERR_NOT_FOUND, "Directory %s doesn't exist", path);
    return nerr_raise_errno (NERR_IO, "Unable to open directory %s", path);
  }
  while ((de = readdir (dp)) != NULL)
  {
    l = strlen (de->d_name);
    if (l>4 && !strcmp (de->d_name+l-4, ",log"))
    {
      f = (char *) malloc ((l-3) * sizeof(char));
      if (f == NULL)
      {
	uListDestroy (&files, ULIST_FREE);
	closedir(dp);
	return nerr_raise (NERR_NOMEM, 
	    "Unable to allocate memory for filename %s", de->d_name);
      }
      strncpy (f, de->d_name, l-4);
      f[l-4] = '\0';
      err = uListAppend (files, f);
      if (err)
      {
	free (f);
	uListDestroy (&files, ULIST_FREE);
	closedir(dp);
	return nerr_pass (err);
      }
    }
  }
  *list = files;
  closedir(dp);

  return STATUS_OK;
}

