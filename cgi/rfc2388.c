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

/* rfc2388 defines multipart/form-data which is primarily used for
 * HTTP file upload
 */

#include "cs_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/neo_str.h"
#include "cgi.h"
#include "cgiwrap.h"

static NEOERR * _header_value (char *hdr, char **val)
{
  char *p, *q;
  int l;

  *val = NULL;

  p = hdr;
  while (*p && isspace(*p)) p++;
  q = p;
  while (*q && !isspace(*q) && *q != ';') q++;
  if (!*p || p == q) return STATUS_OK;

  l = q - p;
  *val = (char *) malloc (l+1);
  if (*val == NULL)
    return nerr_raise (NERR_NOMEM, "Unable to allocate space for val");
  memcpy (*val, p, l);
  (*val)[l] = '\0';

  return STATUS_OK;
}

static NEOERR * _header_attr (char *hdr, char *attr, char **val)
{
  char *p, *k, *v;
  int found = 0;
  int l, al;
  char *r;

  *val = NULL;
  l = strlen(attr);

  /* skip value */
  p = hdr;
  while (*p && *p != ';') p++;
  if (!*p) return STATUS_OK;

  p++;
  while(*p && !found)
  {
    while (*p && isspace(*p)) p++;
    if (!*p) return STATUS_OK;
    /* attr name */
    k = p;
    while (*p && !isspace(*p) && *p != ';' && *p != '=') p++;
    if (!*p) return STATUS_OK;
    if (l == (p-k) && !strncasecmp(attr, k, l))
      found = 1;

    while (*p && isspace(*p)) p++;
    if (*p != ';' && *p != '=') return STATUS_OK;
    if (*p == ';')
    {
      if (found)
      {
	*val = strdup ("");
	if (*val == NULL) 
	  return nerr_raise (NERR_NOMEM, "Unable to allocate value");
	return STATUS_OK;
      }
    }
    else 
    {
      p++;
      if (*p == '"')
      {
	v = ++p;
	while (*p && *p != '"') p++;
	al = p-v;
	if (*p) p++;
      }
      else
      {
	v = p;
	while (*p && !isspace(*p) && *p != ';') p++;
	al = p-v;
      }
      if (found)
      {
	r = (char *) malloc (al+1);
	if (r == NULL) 
	  return nerr_raise (NERR_NOMEM, "Unable to allocate value");
	memcpy (r, v, al);
	r[al] = '\0';
	*val = r;
	return STATUS_OK;
      }
    }
    if (*p) p++;
  }
  return STATUS_OK;
}

static NEOERR * _read_line (CGI *cgi, char **s, int *l, int *done)
{
  int ofs = 0;
  char *p;
  int to_read;

  if (cgi->buf == NULL)
  {
    cgi->buflen = 4096;
    cgi->buf = (char *) malloc (sizeof(char) * cgi->buflen);
    if (cgi->buf == NULL)
      return nerr_raise (NERR_NOMEM, "Unable to allocate cgi buf");
  }
  if (cgi->unget)
  {
    cgi->unget = FALSE;
    *s = cgi->last_start;
    *l = cgi->last_length;
    return STATUS_OK;
  }
  if (cgi->found_nl)
  {
    p = memchr (cgi->buf + cgi->nl, '\n', cgi->readlen - cgi->nl);
    if (p) {
      cgi->last_start = *s = cgi->buf + cgi->nl;
      cgi->last_length = *l = p - (cgi->buf + cgi->nl) + 1;
      cgi->found_nl = TRUE;
      cgi->nl = p - cgi->buf + 1;
      return STATUS_OK;
    }
    ofs = cgi->readlen - cgi->nl;
    memmove(cgi->buf, cgi->buf + cgi->nl, ofs);
  }
  // Read either as much buffer space as we have left, or up to 
  // the amount of data remaining according to Content-Length
  // If there is no Content-Length, just use the buffer space, but recognize
  // that it might not work on some servers or cgiwrap implementations.
  // Some servers will close their end of the stdin pipe, so cgiwrap_read
  // will return if we ask for too much.  Techically, not including
  // Content-Length is against the HTTP spec, so we should consider failing
  // earlier if we don't have a length.
  to_read = cgi->buflen - ofs;
  if (cgi->data_expected && (to_read > cgi->data_expected - cgi->data_read))
  {
    to_read = cgi->data_expected - cgi->data_read;
  }
  cgiwrap_read (cgi->buf + ofs, to_read, &(cgi->readlen));
  if (cgi->readlen < 0)
  {
    return nerr_raise_errno (NERR_IO, "POST Read Error");
  }
  if (cgi->readlen == 0)
  {
    *done = 1;
    return STATUS_OK;
  }
  cgi->data_read += cgi->readlen;
  if (cgi->upload_cb)
  {
    if (cgi->upload_cb (cgi, cgi->data_read, cgi->data_expected))
      return nerr_raise (CGIUploadCancelled, "Upload Cancelled");
  }
  cgi->readlen += ofs;
  p = memchr (cgi->buf, '\n', cgi->readlen);
  if (!p)
  {
    cgi->found_nl = FALSE;
    cgi->last_start = *s = cgi->buf;
    cgi->last_length = *l = cgi->readlen;
    return STATUS_OK;
  }
  cgi->last_start = *s = cgi->buf;
  cgi->last_length = *l = p - cgi->buf + 1;
  cgi->found_nl = TRUE;
  cgi->nl = *l;
  return STATUS_OK;
}

static NEOERR * _read_header_line (CGI *cgi, STRING *line, int *done)
{
  NEOERR *err;
  char *s, *p;
  int l;

  err = _read_line (cgi, &s, &l, done);
  if (err) return nerr_pass (err);
  if (*done || (l == 0)) return STATUS_OK;
  if (isspace (s[0])) return STATUS_OK;
  while (l && isspace(s[l-1])) l--;
  err = string_appendn (line, s, l);
  if (err) return nerr_pass (err);

  while (1)
  {
    err = _read_line (cgi, &s, &l, done);
    if (err) break;
    if (l == 0) break;
    if (*done) break;
    if (!(s[0] == ' ' || s[0] == '\t'))
    {
      cgi->unget = TRUE;
      break;
    }
    while (l && isspace(s[l-1])) l--;
    p = s;
    while (*p && isspace(*p) && (p-s < l)) p++;
    err = string_append_char (line, ' ');
    if (err) break;
    err = string_appendn (line, p, l - (p-s));
    if (err) break;
    if (line->len > 50*1024*1024)
    {
      string_clear(line);
      return nerr_raise(NERR_ASSERT, "read_header_line exceeded 50MB");
    }
  }
  return nerr_pass (err);
}

static BOOL _is_boundary (char *boundary, char *s, int l, int *done)
{
  static char *old_boundary = NULL;
  static int bl;

  /* cache the boundary strlen... more pointless optimization by blong */
  if (old_boundary != boundary)
  {
    old_boundary = boundary;
    bl = strlen(boundary);
  }

  if (s[l-1] != '\n')
    return FALSE;
  l--;
  if (s[l-1] == '\r')
    l--;

  if (bl+2 == l && s[0] == '-' && s[1] == '-' && !strncmp (s+2, boundary, bl))
    return TRUE;
  if (bl+4 == l && s[0] == '-' && s[1] == '-' && 
      !strncmp (s+2, boundary, bl) &&
      s[l-1] == '-' && s[l-2] == '-')
  {
    *done = 1;
    return TRUE;
  }
  return FALSE;
}

static NEOERR * _find_boundary (CGI *cgi, char *boundary, int *done)
{
  NEOERR *err;
  char *s;
  int l;

  *done = 0;
  while (1)
  {
    err = _read_line (cgi, &s, &l, done);
    if (err) return nerr_pass (err);
    if ((l == 0) || (*done)) {
      *done = 1;
      return STATUS_OK;
    }
    if (_is_boundary(boundary, s, l, done))
      return STATUS_OK;
  }
  return STATUS_OK;
}

NEOERR *open_upload(CGI *cgi, int unlink_files, FILE **fpw)
{
  NEOERR *err = STATUS_OK;
  FILE *fp;
  char path[_POSIX_PATH_MAX];
  int fd;

  *fpw = NULL;

  snprintf (path, sizeof(path), "%s/cgi_upload.XXXXXX", 
      hdf_get_value(cgi->hdf, "Config.Upload.TmpDir", "/var/tmp"));

  fd = mkstemp(path);
  if (fd == -1)
  {
    return nerr_raise_errno (NERR_SYSTEM, "Unable to open temp file %s", 
	path);
  }

  fp = fdopen (fd, "w+");
  if (fp == NULL)
  {
    close(fd);
    return nerr_raise_errno (NERR_SYSTEM, "Unable to fdopen file %s", path);
  }
  if (unlink_files) unlink(path);
  if (cgi->files == NULL)
  {
    err = uListInit (&(cgi->files), 10, 0);
    if (err)
    {
      fclose(fp);
      return nerr_pass(err);
    }
  }
  err = uListAppend (cgi->files, fp);
  if (err)
  {
    fclose (fp);
    return nerr_pass(err);
  }
  if (!unlink_files) {
    if (cgi->filenames == NULL)
    {
      err = uListInit (&(cgi->filenames), 10, 0);
      if (err)
      {
	fclose(fp);
	return nerr_pass(err);
      }
    }
    err = uListAppend (cgi->filenames, strdup(path));
    if (err)
    {
      fclose (fp);
      return nerr_pass(err);
    }
  }
  *fpw = fp;
  return STATUS_OK;
}

static NEOERR * _read_part (CGI *cgi, char *boundary, int *done)
{
  NEOERR *err = STATUS_OK;
  STRING str;
  HDF *child, *obj = NULL;
  FILE *fp = NULL;
  char buf[256];
  char *p;
  char *name = NULL, *filename = NULL;
  char *type = NULL, *tmp = NULL;
  char *last = NULL;
  int unlink_files = hdf_get_int_value(cgi->hdf, "Config.Upload.Unlink", 1);

  string_init (&str);

  while (1)
  {
    err = _read_header_line (cgi, &str, done);
    if (err) break;
    if (*done) break;
    if (str.buf == NULL || str.buf[0] == '\0') break;
    p = strchr (str.buf, ':');
    if (p)
    {
      *p = '\0';
      if (!strcasecmp(str.buf, "content-disposition"))
      {
	err = _header_attr (p+1, "name", &name);
	if (err) break;
	err = _header_attr (p+1, "filename", &filename);
	if (err) break;
      }
      else if (!strcasecmp(str.buf, "content-type"))
      {
	err = _header_value (p+1, &type);
	if (err) break;
      }
      else if (!strcasecmp(str.buf, "content-encoding"))
      {
	err = _header_value (p+1, &tmp);
	if (err) break;
	if (tmp && strcmp(tmp, "7bit") && strcmp(tmp, "8bit") && 
	    strcmp(tmp, "binary"))
	{
	  free(tmp);
	  err = nerr_raise (NERR_ASSERT, "form-data encoding is not supported");
	  break;
	}
	free(tmp);
      }
    }
    string_set(&str, "");
  }
  if (err) 
  {
    string_clear(&str);
    if (name) free(name);
    if (filename) free(filename);
    if (type) free(type);
    return nerr_pass (err);
  }

  do
  {
    if (filename)
    {
      err = open_upload(cgi, unlink_files, &fp);
      if (err) break;
    }

    string_set(&str, "");
    while (!(*done))
    {
      char *s;
      int l, w;

      err = _read_line (cgi, &s, &l, done);
      if (err) break;
      if (*done || (l == 0)) break;
      if (_is_boundary(boundary, s, l, done)) break;
      if (filename)
      {
	if (last) fwrite (last, sizeof(char), strlen(last), fp);
	if (l > 1 && s[l-1] == '\n' && s[l-2] == '\r')
	{
	  last = "\r\n";
	  l-=2;
	}
	else if (l > 0 && s[l-1] == '\n')
	{
	  last = "\n";
	  l--;
	}
	else last = NULL;
	w = fwrite (s, sizeof(char), l, fp);
	if (w != l)
	{
	  err = nerr_raise_errno (NERR_IO, 
	      "Short write on file %s upload %d < %d", filename, w, l);
	  break;
	}
      }
      else
      {
	err = string_appendn(&str, s, l);
	if (err) break;
      }
    }
    if (err) break;
  } while (0);

  /* Set up the cgi data */
  if (!err)
  {
    do {
      /* FIXME: Hmm, if we've seen the same name here before, what should we do?
       */
      if (filename)
      {
	fseek(fp, 0, SEEK_SET);
	snprintf (buf, sizeof(buf), "Query.%s", name);
	err = hdf_set_value (cgi->hdf, buf, filename);
	if (!err && type)
	{
	  snprintf (buf, sizeof(buf), "Query.%s.Type", name);
	  err = hdf_set_value (cgi->hdf, buf, type);
	}
	if (!err)
	{
	  snprintf (buf, sizeof(buf), "Query.%s.FileHandle", name);
	  err = hdf_set_int_value (cgi->hdf, buf, uListLength(cgi->files));
	}
	if (!err && !unlink_files)
	{
	  char *path;
	  snprintf (buf, sizeof(buf), "Query.%s.FileName", name);
	  err = uListGet(cgi->filenames, uListLength(cgi->filenames)-1, 
	      (void *)&path);
	  if (!err) err = hdf_set_value (cgi->hdf, buf, path);
	}
      }
      else
      {
	snprintf (buf, sizeof(buf), "Query.%s", name);
	while (str.len && isspace(str.buf[str.len-1]))
	{
	  str.buf[str.len-1] = '\0';
	  str.len--;
	}
	if (!(cgi->ignore_empty_form_vars && str.len == 0))
	{
	  /* If we've seen it before... we force it into a list */
	  obj = hdf_get_obj (cgi->hdf, buf);
	  if (obj != NULL)
	  {
	    int i = 0;
	    char buf2[10];
	    char *t;
	    child = hdf_obj_child (obj);
	    if (child == NULL)
	    {
	      t = hdf_obj_value (obj);
	      err = hdf_set_value (obj, "0", t);
	      if (err != STATUS_OK) break;
	      i = 1;
	    }
	    else
	    {
	      while (child != NULL)
	      {
		i++;
		child = hdf_obj_next (child);
		if (err != STATUS_OK) break;
	      }
	      if (err != STATUS_OK) break;
	    }
	    snprintf (buf2, sizeof(buf2), "%d", i);
	    err = hdf_set_value (obj, buf2, str.buf);
	    if (err != STATUS_OK) break;
	  }
	  err = hdf_set_value (cgi->hdf, buf, str.buf);
	}
      }
    } while (0);
  }

  string_clear(&str);
  if (name) free(name);
  if (filename) free(filename);
  if (type) free(type);

  return nerr_pass (err);
}

NEOERR * parse_rfc2388 (CGI *cgi)
{
  NEOERR *err;
  char *ct_hdr;
  char *boundary = NULL;
  int l;
  int done = 0;

  l = hdf_get_int_value (cgi->hdf, "CGI.ContentLength", -1);
  ct_hdr = hdf_get_value (cgi->hdf, "CGI.ContentType", NULL);
  if (ct_hdr == NULL) 
    return nerr_raise (NERR_ASSERT, "No content type header?");

  cgi->data_expected = l;
  cgi->data_read = 0;
  if (cgi->upload_cb)
  {
    if (cgi->upload_cb (cgi, cgi->data_read, cgi->data_expected))
      return nerr_raise (CGIUploadCancelled, "Upload Cancelled");
  }

  err = _header_attr (ct_hdr, "boundary", &boundary);
  if (err) return nerr_pass (err);
  err = _find_boundary(cgi, boundary, &done);
  while (!err && !done)
  {
    err = _read_part (cgi, boundary, &done);
  }

  if (boundary) free(boundary);
  return nerr_pass(err);
}

/* this is here because it gets populated in this file */
FILE *cgi_filehandle (CGI *cgi, const char *form_name)
{
  NEOERR *err;
  FILE *fp;
  char buf[256];
  int n;

  if ((form_name == NULL) || (form_name[0] == '\0'))
  {
    /* if NULL, then its the PUT data we're looking for... */
    n = hdf_get_int_value (cgi->hdf, "PUT.FileHandle", -1);
  }
  else
  {
    snprintf (buf, sizeof(buf), "Query.%s.FileHandle", form_name);
    n = hdf_get_int_value (cgi->hdf, buf, -1);
  }
  if (n == -1) return NULL;
  err = uListGet(cgi->files, n-1, (void *)&fp);
  if (err)
  {
    nerr_ignore(&err);
    return NULL;
  }
  return fp;
}
