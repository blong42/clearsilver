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
 * The wdb is a wrapper around the sleepycat db library which adds
 * a relatively simple data/column definition.  In many respects, this
 * code is way more complicated than it ever needed to be, but it works,
 * so I'm loathe to "fix" it.
 *
 * One of they key features of this is the ability to update the
 * "schema" of the wdb without changing all of the existing rows of
 * data.
 */

#include "cs_config.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <db.h>
#include <ctype.h>

#include "neo_misc.h"
#include "neo_err.h"
#include "dict.h"
#include "ulist.h"
#include "skiplist.h"
#include "wdb.h"


#define DEFN_VERSION_1 "WDB-VERSION-200006301"
#define PACK_VERSION_1 1

static void string_rstrip (char *s)
{
  size_t len;

  len = strlen(s);
  len--;
  while (len > 0 && isspace(s[len]))
  {
    s[len] = '\0';
    len--;
  }
  return; 
}

static int to_hex (unsigned char ch, unsigned char *s)
{
  unsigned int uvalue = ch;

  s[1] = "0123456789ABCDEF"[uvalue % 16];
  uvalue = (uvalue / 16);  s[0] = "0123456789ABCDEF"[uvalue % 16];
  return 0;
}

int Index_hex[128] = {
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
  -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,  -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};

#define hexval(c) Index_hex[(unsigned int)(c)]

/* Encoding means any non-printable characters and : and % */
static NEOERR *wdb_encode_str_alloc (const char *s, char **o)
{
  int x = 0;
  int c = 0;
  char *out;
  unsigned char ch;

  while (s[x])
  {
    ch = (unsigned char) s[x];
    if ((ch < 32) || (ch > 127) || (ch == ':') || (ch == '%') || isspace(ch))
      c++;
    x++;
  }

  out = (char *) malloc (sizeof (char) * (x + c * 3) + 1);
  if (out == NULL)
    return nerr_raise (NERR_NOMEM, 
	"Unable to allocate memory for encoding %s", s);

  x = 0;
  c = 0;

  *o = out;

  while (s[x])
  {
    ch = (unsigned char) s[x];
    if ((ch < 32) || (ch > 127) || (ch == ':') || (ch == '%') || isspace(ch))
    {
      out[c++] = '%';
      to_hex (s[x], &out[c]);
      c+=2;
    }
    else
    {
      out[c++] = s[x];
    }
    x++;
  }
  out[c] = '\0';

  return STATUS_OK;
}

static NEOERR *wdb_decode_str_alloc (const char *s, char **o)
{
  int x = 0;
  int c = 0;
  char *out;
  unsigned char ch;


  x = strlen(s);
  /* Overkill, the decoded string will be smaller */
  out = (char *) malloc (sizeof (char) * (x + 1));
  if (out == NULL)
    return nerr_raise (NERR_NOMEM, 
	"Unable to allocate memory for decoding %s", s);

  x = 0;
  c = 0;

  while (s[x])
  {
    if (s[x] == '%')
    {
      x++;
      ch = hexval(s[x]) << 4;
      x++;
      ch |= hexval(s[x]);
      out[c++] = ch;
    }
    else
    {
      out[c++] = s[x];
    }
    x++;
  }
  out[c] = '\0';
  *o = out;

  return STATUS_OK;
}

static void free_cb (void *value, void *rock)
{
  free (value);
}

static void free_col_cb (void *value, void *rock)
{
  WDBColumn *col;

  col = (WDBColumn *)value;

  free (col->name);
  free (col);
}

static NEOERR *wdb_alloc (WDB **wdb, int flags)
{
  WDB *my_wdb;
  NEOERR *err = STATUS_OK;

  my_wdb = (WDB *) calloc (1, sizeof (WDB));

  if (my_wdb == NULL)
    return nerr_raise (NERR_NOMEM, "Unable to allocate memory for WDB");

  do
  {
    err = dictCreate (&(my_wdb->attrs), 0, 2, 5, 0, 0, free_cb, NULL);
    if (err != STATUS_OK) break;
    err = dictCreate (&(my_wdb->cols), 0, 2, 5, 0, 0, free_col_cb, NULL);
    if (err != STATUS_OK) break;
    err = uListInit (&(my_wdb->cols_l), 0, 0);
    if (err != STATUS_OK) break;
    err = skipNewList(&(my_wdb->ondisk), 0, 4, 2, 0, NULL, NULL);
    if (err != STATUS_OK) break;

    *wdb = my_wdb;

    return STATUS_OK;
  } while (0);

  wdb_destroy(&my_wdb);
  return nerr_pass (err);
}


#define STATE_REQUIRED 1
#define STATE_ATTRIBUTES 2
#define STATE_COLUMN_DEF 3

static NEOERR *wdb_load_defn_v1 (WDB *wdb, FILE *fp)
{
  char line[1024];
  int state = 1;
  char *k, *v;
  NEOERR *err = STATUS_OK;
  int colindex = 1;
  WDBColumn *col;

  while (fgets(line, sizeof(line), fp) != NULL)
  {
    string_rstrip(line);
    switch (state)
    {
      case STATE_REQUIRED:
	if (!strcmp(line, "attributes"))
	  state = STATE_ATTRIBUTES;
	else if (!strcmp(line, "columns"))
	  state = STATE_COLUMN_DEF;
	else
	{
	  k = line;
	  v = strchr(line, ':');
	  /* HACK */
	  if (!strcmp(k, "name") && ((v == NULL) || (v[1] == '\0')))
	  {
	    v = "dNone";
	  }
	  else 
	  {
	    if (v == NULL)
	      return nerr_raise (NERR_PARSE, "Error parsing %s", line);
	    if (v[1] == '\0')
	      return nerr_raise (NERR_PARSE, "Error parsing %s", line);
	  }
	  v[0] = '\0';
	  v++;
	  if (!strcmp(k, "key"))
	  { 
	    err = wdb_decode_str_alloc (v, &(wdb->key));
	    if (err) return nerr_pass(err);
	  } 
	  else if (!strcmp(k, "name"))
	  {
	    err = wdb_decode_str_alloc (v, &(wdb->name));
	    if (err) return nerr_pass(err);
	  }
	  else if (!strcmp(k, "ondisk"))
	  {
	    wdb->last_ondisk = atoi (v);
	  }
	}
	break;
      case STATE_ATTRIBUTES:
	if (!strcmp(line, "columns"))
	  state = STATE_COLUMN_DEF;
	else
	{
	  k = line;
	  v = strchr(line, ':');
	  if (v == NULL)
	    return nerr_raise (NERR_PARSE, "Error parsing %s", line);
	  v[0] = '\0';
	  v++;
	  err = wdb_decode_str_alloc (k, &k);
	  if (err) return nerr_pass(err);
	  err = wdb_decode_str_alloc (v, &v);
	  if (err) return nerr_pass(err);
	  err = dictSetValue(wdb->attrs, k, v);
	  free(k);
	  if (err)
	    return nerr_pass_ctx(err, "Error parsing %s", line);
	}
	break;
      case STATE_COLUMN_DEF:
	k = line;
	v = strchr(line, ':');
	if (v == NULL)
	  return nerr_raise (NERR_PARSE, "Error parsing %s", line);
	if (v[1] == '\0')
	  return nerr_raise (NERR_PARSE, "Error parsing %s", line);
	v[0] = '\0';
	v++;
	err = wdb_decode_str_alloc (k, &k);
	if (err) return nerr_pass(err);
	col = (WDBColumn *) calloc (1, sizeof (WDBColumn));
	col->name = k;
	col->inmem_index = colindex++;
	col->type = *v;
	v+=2;
	col->ondisk_index = atoi(v);
	err = dictSetValue(wdb->cols, k, col);
	if (err)
	  return nerr_raise (NERR_PARSE, "Error parsing %s", line);
	err = uListAppend(wdb->cols_l, col);
	if (err) return nerr_pass(err);
	/* stupid skiplist will assert */
	if (col->ondisk_index == 0)
	{
	  return nerr_raise (NERR_ASSERT, "Invalid ondisk mapping for %s", k);
	}
	err = skipInsert (wdb->ondisk, col->ondisk_index, 
	    (void *)(col->inmem_index), 0);
	if (err)
	  return nerr_pass_ctx(err, "Unable to update ondisk mapping for %s", k);
	break;
      default:
	return nerr_raise (NERR_ASSERT, "Invalid state %d", state);
    }
  }
  return STATUS_OK;
}

static NEOERR *wdb_save_defn_v1 (WDB *wdb, FILE *fp)
{
  NEOERR *err = STATUS_OK;
  WDBColumn *col;
  char *s = NULL;
  char *key = NULL;
  int r, x, len;
  char *k = NULL;
  char *v = NULL;

  /* Write version string */
  r = fprintf (fp, "%s\n", DEFN_VERSION_1);
  if (!r) goto save_err;

  err = wdb_encode_str_alloc (wdb->name, &s);
  if (err) goto save_err;
  r = fprintf (fp, "name:%s\n", s);
  if (!r) goto save_err;
  free (s);

  err = wdb_encode_str_alloc (wdb->key, &s);
  if (err != STATUS_OK) goto save_err;
  r = fprintf (fp, "key:%s\n", s);
  if (!r) goto save_err;
  free (s);
  s = NULL;

  r = fprintf (fp, "ondisk:%d\n", wdb->last_ondisk);
  if (!r) goto save_err;

  r = fprintf (fp, "attributes\n");
  if (!r) goto save_err;
  key = NULL;
  s = (char *) dictNext (wdb->attrs, &key, NULL);
  while (s)
  {
    err = wdb_encode_str_alloc (key, &k);
    if (err != STATUS_OK) goto save_err;
    err = wdb_encode_str_alloc (s, &v);
    if (err != STATUS_OK) goto save_err;
    r = fprintf (fp, "%s:%s\n", k, v);
    if (!r) goto save_err;
    free (k);
    free (v);
    k = NULL;
    v = NULL;
    s = (char *) dictNext (wdb->attrs, &key, NULL);
  }
  s = NULL;

  r = fprintf (fp, "columns\n");
  if (!r) goto save_err;

  len = uListLength(wdb->cols_l);

  for (x = 0; x < len; x++)
  {
    err = uListGet (wdb->cols_l, x, (void *)&col);
    if (err) goto save_err;
    err = wdb_encode_str_alloc (col->name, &s);
    if (err != STATUS_OK) goto save_err;
    r = fprintf (fp, "%s:%c:%d\n", s, col->type, col->ondisk_index);
    if (!r) goto save_err;
    free(s);
    s = NULL;
  }

  return STATUS_OK;

save_err:
  if (s != NULL) free (s);
  if (k != NULL) free (k);
  if (v != NULL) free (v);
  if (err == STATUS_OK) return nerr_pass(err);
  return nerr_raise (r, "Unable to save defn");
}

static NEOERR *wdb_load_defn (WDB *wdb, const char *name)
{
  char path[_POSIX_PATH_MAX];
  char line[1024];
  FILE *fp;
  NEOERR *err = STATUS_OK;

  snprintf (path, sizeof(path), "%s.wdf", name);
  fp = fopen (path, "r");
  if (fp == NULL)
  {
    if (errno == ENOENT)
      return nerr_raise (NERR_NOT_FOUND, "Unable to open defn %s", name);
    return nerr_raise_errno (NERR_IO, "Unable to open defn %s", name);
  }

  /* Read Version string */
  if (fgets (line, sizeof(line), fp) == NULL)
  {
    fclose(fp);
    return nerr_raise_errno (NERR_IO, "Unable to read defn %s", name);
  }
  string_rstrip(line);

  if (!strcmp(line, DEFN_VERSION_1))
  {
    err = wdb_load_defn_v1(wdb, fp);
    fclose(fp);
    if (err) return nerr_pass(err);
  }
  else
  {
    fclose(fp);
    return nerr_raise (NERR_ASSERT, "Unknown defn version %s: %s", line, name);
  }

  wdb->table_version = rand();

  return STATUS_OK;
}

static NEOERR *wdb_save_defn (WDB *wdb, const char *name)
{
  char path[_POSIX_PATH_MAX];
  char path2[_POSIX_PATH_MAX];
  FILE *fp;
  NEOERR *err = STATUS_OK;
  int r;

  snprintf (path, sizeof(path), "%s.wdf.new", name);
  snprintf (path2, sizeof(path2), "%s.wdf", name);
  fp = fopen (path, "w");
  if (fp == NULL)
    return nerr_raise_errno (NERR_IO, "Unable to open defn %s", name);

  err = wdb_save_defn_v1 (wdb, fp);
  fclose (fp);
  if (err != STATUS_OK) 
  {
    unlink (path);
    return nerr_pass (err);
  }

  r = unlink (path2);
  if (r == -1 && errno != ENOENT)
    return nerr_raise_errno (NERR_IO, "Unable to unlink %s", path2);
  r = link (path, path2);
  if (r == -1)
    return nerr_raise_errno (NERR_IO, "Unable to link %s to %s", path, path2);
  r = unlink (path);

  wdb->defn_dirty = 0;
  wdb->table_version = rand();

  return STATUS_OK;
}

NEOERR *wdb_open (WDB **wdb, const char *name, int flags)
{
  WDB *my_wdb;
  char path[_POSIX_PATH_MAX];
  NEOERR *err = STATUS_OK;
  int r;

  *wdb = NULL;

  err = wdb_alloc (&my_wdb, flags);
  if (err) return nerr_pass(err);

  my_wdb->path = strdup (name);
  if (err)
  {
    wdb_destroy (&my_wdb);
    return nerr_pass(err);
  }

  err = wdb_load_defn (my_wdb, name);
  if (err)
  {
    wdb_destroy (&my_wdb);
    return nerr_pass(err);
  }

  snprintf (path, sizeof(path), "%s.wdb", name);
  r = db_open(path, DB_BTREE, 0, 0, NULL, NULL, &(my_wdb->db));
  if (r)
  {
    wdb_destroy (&my_wdb);
    return nerr_raise (NERR_DB, "Unable to open database %s: %d", name, r);
  }

  *wdb = my_wdb;

  return STATUS_OK;
}

NEOERR *wdb_save (WDB *wdb)
{
  if (wdb->defn_dirty)
  {
    wdb_save_defn (wdb, wdb->path);
  }
  return STATUS_OK;
}

void wdb_destroy (WDB **wdb)
{
  WDB *my_wdb;
    
  my_wdb = *wdb;

  if (my_wdb == NULL) return;

  if (my_wdb->defn_dirty)
  {
    wdb_save_defn (my_wdb, my_wdb->path);
  }

  if (my_wdb->attrs != NULL)
  {
    dictDestroy (my_wdb->attrs);
  }

  if (my_wdb->cols != NULL)
  {
    dictDestroy (my_wdb->cols);
  }

  if (my_wdb->cols_l != NULL)
  {
    uListDestroy(&(my_wdb->cols_l), 0);
  }

  if (my_wdb->ondisk != NULL)
  {
    skipFreeList(my_wdb->ondisk);
  }

  if (my_wdb->db != NULL)
  {
    my_wdb->db->close (my_wdb->db, 0);
    my_wdb->db = NULL;
  }

  if (my_wdb->path != NULL)
  {
    free(my_wdb->path);
    my_wdb->path = NULL;
  }
  if (my_wdb->name != NULL)
  {
    free(my_wdb->name);
    my_wdb->name = NULL;
  }
  if (my_wdb->key != NULL)
  {
    free(my_wdb->key);
    my_wdb->key = NULL;
  }

  free (my_wdb);
  *wdb = NULL;

  return;
}

#define PACK_UB4(pdata, plen, pmax, pn) \
{ \
  if (plen + 4 > pmax) \
  { \
    pmax *= 2; \
    pdata = realloc ((void *)pdata, pmax); \
    if (pdata == NULL) goto pack_err; \
  } \
  pdata[plen++] = (0x0ff & (pn >> 0)); \
  pdata[plen++] = (0x0ff & (pn >> 8)); \
  pdata[plen++] = (0x0ff & (pn >> 16)); \
  pdata[plen++] = (0x0ff & (pn >> 24)); \
}

#define UNPACK_UB4(pdata, plen, pn, pd) \
{ \
  if (pn + 4 > plen) \
    goto pack_err; \
  pd = ((0x0ff & pdata[pn+0])<<0) | ((0x0ff & pdata[pn+1])<<8) | \
       ((0x0ff & pdata[pn+2])<<16) | ((0x0ff & pdata[pn+3])<<24); \
  pn+=4; \
}

#define PACK_BYTE(pdata, plen, pmax, pn) \
{ \
  if (plen + 1 > pmax) \
  { \
    pmax *= 2; \
    pdata = realloc ((void *)pdata, pmax); \
    if (pdata == NULL) goto pack_err; \
  } \
  pdata[plen++] = (0x0ff & (pn >> 0)); \
}

#define UNPACK_BYTE(pdata, plen, pn, pd) \
{ \
  if (pn + 1 > plen) \
    goto pack_err; \
  pd = pdata[pn++]; \
}

#define PACK_STRING(pdata, plen, pmax, dl, ds) \
{ \
  if (plen + 4 + dl > pmax) \
  { \
    while (plen + 4 + dl > pmax) \
      pmax *= 2; \
    pdata = realloc ((void *)pdata, pmax); \
    if (pdata == NULL) goto pack_err; \
  } \
  pdata[plen++] = (0x0ff & (dl >> 0)); \
  pdata[plen++] = (0x0ff & (dl >> 8)); \
  pdata[plen++] = (0x0ff & (dl >> 16)); \
  pdata[plen++] = (0x0ff & (dl >> 24)); \
  memcpy (&pdata[plen], ds, dl); \
  plen+=dl;\
}

#define UNPACK_STRING(pdata, plen, pn, ps) \
{ \
  int pl; \
  if (pn + 4 > plen) \
    goto pack_err; \
  pl = ((0x0ff & pdata[pn+0])<<0) | ((0x0ff & pdata[pn+1])<<8) | \
       ((0x0ff & pdata[pn+2])<<16) | ((0x0ff & pdata[pn+3])<<24); \
  pn+=4; \
  if (pl) \
  { \
    ps = (char *)malloc(sizeof(char)*(pl+1)); \
    if (ps == NULL) \
      goto pack_err; \
    memcpy (ps, &pdata[pn], pl); \
    ps[pl] = '\0'; \
    pn += pl; \
  } else { \
    ps = NULL; \
  } \
}

/* A VERSION_1 Row consists of the following data:
 * UB4 VERSION
 * UB4 DATA COUNT
 * DATA where
 *   UB4 ONDISK INDEX
 *   UB1 TYPE 
 *   if INT, then UB4 
 *   if STR, then UB4 length and length UB1s
 */

static NEOERR *pack_row (WDB *wdb, WDBRow *row, void **rdata, int *rdlen)
{
  char *data;
  int x, len, dlen, dmax;
  char *s;
  int n;
  WDBColumn *col;
  NEOERR *err;

  *rdata = NULL;
  *rdlen = 0;
  /* allocate */
  data = (char *)malloc(sizeof (char) * 1024);
  if (data == NULL)
    return nerr_raise (NERR_NOMEM, "Unable to allocate memory to pack row");

  dmax = 1024;
  dlen = 0;

  PACK_UB4 (data, dlen, dmax, PACK_VERSION_1);
/*  PACK_UB4 (data, dlen, dmax, time(NULL)); */

  len = uListLength(wdb->cols_l);
  if (len > row->data_count)
    len = row->data_count;
  PACK_UB4 (data, dlen, dmax, len);

  for (x = 0; x < len; x++)
  {
    err = uListGet (wdb->cols_l, x, (void *)&col);
    if (err) goto pack_err;
    PACK_UB4 (data, dlen, dmax, col->ondisk_index);
    PACK_BYTE (data, dlen, dmax, col->type);
    switch (col->type)
    {
      case WDB_TYPE_INT:
	n = (int)(row->data[x]);
	PACK_UB4 (data, dlen, dmax, n);
	break;
      case WDB_TYPE_STR:
	s = (char *)(row->data[x]);
	if (s == NULL)
	{
	  s = "";
	}
	n = strlen(s);
	PACK_STRING (data, dlen, dmax, n, s);
	break;
      default:
	free (data);
	return nerr_raise (NERR_ASSERT, "Unknown type %d", col->type);
    }
  }

  *rdata = data;
  *rdlen = dlen;
  return STATUS_OK;

pack_err:
  if (data != NULL)
    free (data);
  if (err == STATUS_OK)
    return nerr_raise(NERR_NOMEM, "Unable to allocate memory for pack_row");
  return nerr_pass(err);
}

static NEOERR *unpack_row (WDB *wdb, void *rdata, int dlen, WDBRow *row)
{
  unsigned char *data = rdata;
  int version, n;
  int count, x, ondisk_index, type, d_int, inmem_index;
  char *s;

  n = 0;

  UNPACK_UB4(data, dlen, n, version);

  switch (version)
  {
    case PACK_VERSION_1:
      UNPACK_UB4(data, dlen, n, count);
      for (x = 0; x<count; x++)
      {
	UNPACK_UB4 (data, dlen, n, ondisk_index);
	UNPACK_BYTE (data, dlen, n, type);
	inmem_index = (int) skipSearch (wdb->ondisk, ondisk_index, NULL);

	switch (type)
	{
	  case WDB_TYPE_INT:
	    UNPACK_UB4 (data, dlen, n, d_int);
	    if (inmem_index != 0)
	      row->data[inmem_index-1] = (void *) d_int;
	    break;
	  case WDB_TYPE_STR:
	    UNPACK_STRING (data, dlen, n, s);
	    if (inmem_index != 0)
	      row->data[inmem_index-1] = s;
	    break;
	  default:
	    return nerr_raise (NERR_ASSERT, "Unknown type %d for col %d", type, ondisk_index);
	}
      }
      break;
    default:
      return nerr_raise (NERR_ASSERT, "Unknown version %d", version);
  }
      
  return STATUS_OK;
pack_err:
  return nerr_raise(NERR_PARSE, "Unable to unpack row %s", row->key_value);
}

NEOERR *wdb_column_insert (WDB *wdb, int loc, const char *key, char type)
{
  NEOERR *err;
  WDBColumn *col, *ocol;
  int x, len;

  col = (WDBColumn *) dictSearch (wdb->cols, key, NULL);

  if (col != NULL)
    return nerr_raise (NERR_DUPLICATE, 
	"Duplicate key %s:%d", key, col->inmem_index);

  col = (WDBColumn *) calloc (1, sizeof (WDBColumn));
  if (col == NULL)
  {
    return nerr_raise (NERR_NOMEM, 
	"Unable to allocate memory for creation of col %s:%d", key, loc);
  }

  col->name = strdup(key);
  if (col->name == NULL)
  {
    free(col);
    return nerr_raise (NERR_NOMEM, 
	"Unable to allocate memory for creation of col %s:%d", key, loc);
  }
  col->type = type;
  col->ondisk_index = wdb->last_ondisk++;
  /* -1 == append */
  if (loc == -1)
  {
    err = dictSetValue(wdb->cols, key, col);
    if (err)
    {
      free (col->name);
      free (col);
      return nerr_pass_ctx (err,
	  "Unable to insert for creation of col %s:%d", key, loc);
    }
    err = uListAppend (wdb->cols_l, (void *)col);
    if (err) return nerr_pass(err);
    x = uListLength (wdb->cols_l);
    col->inmem_index = x;
    err = skipInsert (wdb->ondisk, col->ondisk_index, 
	            (void *)(col->inmem_index), 0);
    if (err)
      return nerr_pass_ctx (err, "Unable to update ondisk mapping for %s", key);
  }
  else
  {
    /* We are inserting this in middle, so the skipList ondisk is now
     * invalid, as is the inmem_index for all cols */
    err = dictSetValue(wdb->cols, key, col);
    if (err)
    {
      free (col->name);
      free (col);
      return nerr_pass_ctx (err,
	  "Unable to insert for creation of col %s:%d", key, loc);
    }
    err = uListInsert (wdb->cols_l, loc, (void *)col);
    if (err) return nerr_pass(err);
    len = uListLength (wdb->cols_l);
    /* Fix up inmem_index and ondisk skipList */
    for (x = 0; x < len; x++)
    {
      err = uListGet (wdb->cols_l, x, (void *)&ocol);
      if (err) return nerr_pass(err);
      ocol->inmem_index = x + 1;
      err = skipInsert (wdb->ondisk, ocol->ondisk_index, 
	              (void *)(ocol->inmem_index), TRUE);
      if (err)
	return nerr_pass_ctx (err, "Unable to update ondisk mapping for %s", key);
    }
  }

  wdb->defn_dirty = 1;
  wdb->table_version = rand();

  return STATUS_OK;
}

NEOERR *wdb_column_update (WDB *wdb, const char *oldkey, const char *newkey)
{
  WDBColumn *ocol, *col;
  WDBColumn *vcol;
  NEOERR *err = STATUS_OK;
  int x, len, r;

  ocol = (WDBColumn *) dictSearch (wdb->cols, oldkey, NULL);

  if (ocol == NULL)
    return nerr_raise (NERR_NOT_FOUND, 
	"Unable to find column for key %s", oldkey);

  col = (WDBColumn *) calloc (1, sizeof (WDBColumn));
  if (col == NULL)
  {
    return nerr_raise (NERR_NOMEM, 
	"Unable to allocate memory for column update %s", newkey);
  }

  *col = *ocol;
  col->name = strdup(newkey);
  if (col->name == NULL)
  {
    free(col);
    return nerr_raise (NERR_NOMEM, 
	"Unable to allocate memory for column update %s", oldkey);
  }
  len = uListLength(wdb->cols_l);
  for (x = 0; x < len; x++)
  {
    err = uListGet (wdb->cols_l, x, (void *)&vcol);
    if (err) return nerr_pass(err);
    if (!strcmp(vcol->name, oldkey))
    {
      err = uListSet (wdb->cols_l, x, (void *)col);
      if (err) return nerr_pass(err);
      break;
    }
  }
  if (x>len)
  {
    return nerr_raise (NERR_ASSERT, "Unable to find cols_l for key %s", oldkey);
  }

  r = dictRemove (wdb->cols, oldkey); /* Only failure is key not found */
  err = dictSetValue(wdb->cols, newkey, col);
  if (err)
  {
    free (col->name);
    free (col);
    return nerr_pass_ctx (err,
	"Unable to insert for update of col %s->%s", oldkey, newkey);
  }

  wdb->defn_dirty = 1;
  wdb->table_version = rand();

  return STATUS_OK;
}

NEOERR *wdb_column_delete (WDB *wdb, const char *name)
{
  WDBColumn *col;
  NEOERR *err = STATUS_OK;
  int len, x, r;

  len = uListLength(wdb->cols_l);
  for (x = 0; x < len; x++)
  {
    err = uListGet (wdb->cols_l, x, (void *)&col);
    if (err) return nerr_pass(err);
    if (!strcmp(col->name, name))
    {
      err = uListDelete (wdb->cols_l, x, NULL);
      if (err) return nerr_pass(err);
      break;
    }
  }

  r = dictRemove (wdb->cols, name); /* Only failure is key not found */
  if (!r)
  {
    return nerr_raise (NERR_NOT_FOUND, 
	"Unable to find column for key %s", name);
  }
  wdb->defn_dirty = 1;
  wdb->table_version = rand();

  return STATUS_OK;
}

NEOERR *wdb_column_exchange (WDB *wdb, const char *key1, const char *key2)
{
  return nerr_raise (NERR_ASSERT,
      "wdb_column_exchange: Not Implemented");
}

/* Not that there's that much point in changing the key name ... */
NEOERR *wdb_update (WDB *wdb, const char *name, const char *key)
{
  if (name != NULL && strcmp(wdb->name, name))
  {
    if (wdb->name != NULL)
      free(wdb->name);
    wdb->name = strdup(name);
    if (wdb->name == NULL)
      return nerr_raise (NERR_NOMEM, 
	  "Unable to allocate memory to update name to %s", name);
    wdb->defn_dirty = 1;
    wdb->table_version = rand();
  }
  if (key != NULL && strcmp(wdb->key, key))
  {
    if (wdb->key != NULL)
      free(wdb->key);
    wdb->key = strdup(key);
    if (wdb->key == NULL)
    {
      wdb->defn_dirty = 0;
      return nerr_raise (NERR_NOMEM, 
	  "Unable to allocate memory to update key to %s", key);
    }
    wdb->defn_dirty = 1;
    wdb->table_version = rand();
  }
  return STATUS_OK;
}

NEOERR *wdb_create (WDB **wdb, const char *path, const char *name, 
                    const char *key, ULIST *col_def, int flags)
{
  WDB *my_wdb;
  char d_path[_POSIX_PATH_MAX];
  NEOERR *err = STATUS_OK;
  int x, len, r;
  char *s;

  *wdb = NULL;

  err = wdb_alloc (&my_wdb, flags);
  if (err) return nerr_pass(err);

  my_wdb->name = strdup (name);
  my_wdb->key = strdup (key);
  my_wdb->path = strdup(path);
  if (my_wdb->name == NULL || my_wdb->key == NULL || my_wdb->path == NULL)
  {
    wdb_destroy (&my_wdb);
    return nerr_raise (NERR_NOMEM, 
	"Unable to allocate memory for creation of %s", name);
  }

  /* ondisk must start at one because of skipList */
  my_wdb->last_ondisk = 1;   
  len = uListLength(col_def);
  for (x = 0; x < len; x++)
  {
    err = uListGet (col_def, x, (void *)&s);
    if (err) 
    {
      wdb_destroy (&my_wdb);
      return nerr_pass(err);
    }
    err = wdb_column_insert (my_wdb, -1, s, WDB_TYPE_STR);
    my_wdb->defn_dirty = 0; /* So we don't save on error destroy */
    if (err) 
    {
      wdb_destroy (&my_wdb);
      return nerr_pass(err);
    }
  }

  err = wdb_save_defn (my_wdb, path);
  if (err)
  {
    wdb_destroy (&my_wdb);
    return nerr_pass(err);
  }

  snprintf (d_path, sizeof(d_path), "%s.wdb", path);
  r = db_open(d_path, DB_BTREE, DB_CREATE | DB_TRUNCATE, 0, NULL, NULL, &(my_wdb->db));
  if (r)
  {
    wdb_destroy (&my_wdb);
    return nerr_raise (NERR_DB, "Unable to create db file %s: %d", d_path, r);
  }

  *wdb = my_wdb;

  return STATUS_OK;
}

NEOERR *wdb_attr_next (WDB *wdb, char **key, char **value)
{
  *value = (char *) dictNext (wdb->attrs, key, NULL);

  return STATUS_OK;
}

NEOERR *wdb_attr_get (WDB *wdb, const char *key, char **value)
{
  void *v;

  v = dictSearch (wdb->attrs, key, NULL);

  if (v == NULL)
    return nerr_raise (NERR_NOT_FOUND, "Unable to find attr %s", key);

  *value = (char *)v;

  return STATUS_OK;
}

NEOERR *wdb_attr_set (WDB *wdb, const char *key, const char *value)
{
  NEOERR *err = STATUS_OK;
  char *v;

  v = strdup(value);
  if (v == NULL)
    return nerr_raise (NERR_NOMEM, "No memory for new attr");

  err = dictSetValue(wdb->attrs, key, v);
  if (err)
    return nerr_pass_ctx (err, "Unable to set attr %s", key);

  wdb->defn_dirty = 1;

  return STATUS_OK;
}

NEOERR *wdbr_get (WDB *wdb, WDBRow *row, const char *key, void **value)
{
  WDBColumn *col;
  void *v;

  col = (WDBColumn *) dictSearch (wdb->cols, key, NULL);

  if (col == NULL)
    return nerr_raise (NERR_NOT_FOUND, "Unable to find key %s", key);

  if (col->inmem_index-1 > row->data_count)
    return nerr_raise (NERR_ASSERT, "Index for key %s is greater than row data, was table altered?", key);
  
  v = row->data[col->inmem_index-1];

  *value = v;

  return STATUS_OK;
}

NEOERR *wdbr_set (WDB *wdb, WDBRow *row, const char *key, void *value)
{
  WDBColumn *col;

  col = (WDBColumn *) dictSearch (wdb->cols, key, NULL);

  if (col == NULL)
    return nerr_raise (NERR_NOT_FOUND, "Unable to find key %s", key);

  if (col->inmem_index-1 > row->data_count)
    return nerr_raise (NERR_ASSERT, "Index for key %s is greater than row data, was table altered?", key);

  if (col->type == WDB_TYPE_STR && row->data[col->inmem_index-1] != NULL)
  {
    free (row->data[col->inmem_index-1]);
  }
  row->data[col->inmem_index-1] = value;

  return STATUS_OK;
}

static NEOERR *alloc_row (WDB *wdb, WDBRow **row)
{
  WDBRow *my_row;
  int len;

  *row = NULL;

  len = uListLength (wdb->cols_l);

  my_row = (WDBRow *) calloc (1, sizeof (WDBRow) + len * (sizeof (void *)));
  if (my_row == NULL)
    return nerr_raise (NERR_NOMEM, "No memory for new row");

  my_row->data_count = len;
  my_row->table_version = wdb->table_version;
 
  *row = my_row;

  return STATUS_OK;
}

NEOERR *wdbr_destroy (WDB *wdb, WDBRow **row)
{
  WDBColumn *col;
  WDBRow *my_row;
  int len, x;
  NEOERR *err;

  err = STATUS_OK;
  if (*row == NULL)
    return err;

  my_row = *row;

  /* Verify this row maps to this table, or else we could do something
   * bad */

  if (wdb->table_version != my_row->table_version)
    return nerr_raise (NERR_ASSERT, "Row %s doesn't match current table", my_row->key_value);

  if (my_row->key_value != NULL)
    free (my_row->key_value);

  len = uListLength(wdb->cols_l);

  for (x = 0; x < len; x++)
  {
    if (my_row->data[x] != NULL)
    {
      err = uListGet (wdb->cols_l, x, (void *)&col);
      if (err) break;
      switch (col->type)
      {
	case WDB_TYPE_INT:
	  break;
	case WDB_TYPE_STR:
	  free (my_row->data[x]);
	  break;
	default:
	  return nerr_raise (NERR_ASSERT, "Unknown type %d", col->type);
      }
    }
  }

  free (my_row);
  *row = NULL;

  return nerr_pass(err);
}

NEOERR *wdbr_lookup (WDB *wdb, const char *key, WDBRow **row)
{
  DBT dkey, data;
  NEOERR *err = STATUS_OK;
  WDBRow *my_row;
  int r;

  *row = NULL;

  memset(&dkey, 0, sizeof(dkey));
  memset(&data, 0, sizeof(data));

  dkey.flags = DB_DBT_USERMEM;
  data.flags = DB_DBT_MALLOC;

  dkey.data = (void *)key;
  dkey.size = strlen(key);

  r = wdb->db->get (wdb->db, NULL, &dkey, &data, 0);
  if (r == DB_NOTFOUND)
    return nerr_raise (NERR_NOT_FOUND, "Unable to find key %s", key);
  else if (r)
    return nerr_raise (NERR_DB, "Error retrieving key %s: %d", key, r);

  /* allocate row */
  err = alloc_row (wdb, &my_row);
  if (err != STATUS_OK)
  {
    free (data.data);
    return nerr_pass(err);
  }

  my_row->key_value = strdup(key);
  if (my_row->key_value == NULL)
  {
    free (data.data);
    free (my_row);
    return nerr_raise (NERR_NOMEM, "No memory for new row");
  }

  /* unpack row */
  err = unpack_row (wdb, data.data, data.size, my_row);
  free (data.data);
  if (err)
  {
    free (my_row);
    return nerr_pass(err);
  }

  *row = my_row;

  return STATUS_OK;
}

NEOERR *wdbr_create (WDB *wdb, const char *key, WDBRow **row)
{
  WDBRow *my_row;
  NEOERR *err = STATUS_OK;

  *row = NULL;

  /* allocate row */
  err = alloc_row (wdb, &my_row);
  if (err) return nerr_pass(err);

  my_row->key_value = strdup(key);
  if (my_row->key_value == NULL)
  {
    wdbr_destroy (wdb, &my_row);
    return nerr_raise (NERR_NOMEM, "No memory for new row");
  }

  *row = my_row;

  return STATUS_OK;
}

NEOERR *wdbr_save (WDB *wdb, WDBRow *row, int flags)
{
  DBT dkey, data;
  int dflags = 0;
  NEOERR *err = STATUS_OK;
  int r;

  memset(&dkey, 0, sizeof(dkey));
  memset(&data, 0, sizeof(data));

  dkey.data = row->key_value;
  dkey.size = strlen(row->key_value);

  err = pack_row (wdb, row, &(data.data), &data.size);
  if (err != STATUS_OK) return nerr_pass(err);

  if (flags & WDBR_INSERT)
  {
    dflags = DB_NOOVERWRITE;
  }

  r = wdb->db->put (wdb->db, NULL, &dkey, &data, dflags);
  free (data.data);
  if (r == DB_KEYEXIST)
    return nerr_raise (NERR_DUPLICATE, "Key %s already exists", row->key_value);
  if (r)
    return nerr_raise (NERR_DB, "Error saving key %s: %d", 
	row->key_value, r);

  return STATUS_OK;
}

NEOERR *wdbr_delete (WDB *wdb, const char *key)
{
  DBT dkey;
  int r;

  memset(&dkey, 0, sizeof(dkey));

  dkey.flags = DB_DBT_USERMEM;

  dkey.data = (void *)key;
  dkey.size = strlen(key);

  r = wdb->db->del (wdb->db, NULL, &dkey, 0);
  if (r == DB_NOTFOUND)
    return nerr_raise (NERR_NOT_FOUND, "Key %s not found", key);
  else if (r)
    return nerr_raise (NERR_DB, "Error deleting key %s: %d", key, r);


  return STATUS_OK;
}

NEOERR *wdbr_dump (WDB *wdb, WDBRow *row)
{
  int x;

  ne_warn ("version: %d", row->table_version);
  ne_warn ("key: %s", row->key_value);
  ne_warn ("count: %d", row->data_count);
  for (x=0; x < row->data_count; x++)
    ne_warn ("data[%d]: %s", x, row->data[x]);

  return STATUS_OK;
}

NEOERR *wdbc_create (WDB *wdb, WDBCursor **cursor)
{
  DBC *db_cursor;
  WDBCursor *new_cursor;
  int r;

  *cursor = NULL;

#if (DB_VERSION_MINOR==4)
  r = (wdb->db)->cursor (wdb->db, NULL, &db_cursor);
#else
  r = (wdb->db)->cursor (wdb->db, NULL, &db_cursor, 0);
#endif
  if (r)
    return nerr_raise (NERR_DB, "Unable to create cursor: %d", r);

  new_cursor = (WDBCursor *) calloc (1, sizeof (WDBCursor));
  if (new_cursor == NULL)
  {
    db_cursor->c_close (db_cursor);
    return nerr_raise (NERR_NOMEM, "Unable to create cursor");
  }

  new_cursor->table_version = wdb->table_version;
  new_cursor->db_cursor = db_cursor;

  *cursor = new_cursor;

  return STATUS_OK;
}

NEOERR *wdbc_destroy (WDB *wdb, WDBCursor **cursor)
{
  if (*cursor != NULL)
  {
    (*cursor)->db_cursor->c_close ((*cursor)->db_cursor);
    free (*cursor);
    *cursor = NULL;
  }
  return STATUS_OK;
}

NEOERR *wdbr_next (WDB *wdb, WDBCursor *cursor, WDBRow **row, int flags)
{
  DBT dkey, data;
  WDBRow *my_row;
  NEOERR *err = STATUS_OK;
  int r;

  *row = NULL;

  if (wdb->table_version != cursor->table_version)
  {
    return nerr_raise (NERR_ASSERT, "Cursor doesn't match database");
  }

  memset(&dkey, 0, sizeof(dkey));
  memset(&data, 0, sizeof(data));
  dkey.flags = DB_DBT_MALLOC;
  data.flags = DB_DBT_MALLOC;

  /* First call */
  if (flags & WDBC_FIRST)
  {
    r = cursor->db_cursor->c_get (cursor->db_cursor, &dkey, &data, DB_FIRST);
    if (r == DB_NOTFOUND)
      return nerr_raise (NERR_NOT_FOUND, "Cursor empty");
    else if (r)
      return nerr_raise (NERR_DB, "Unable to get first item from cursor: %d", 
	  r);
  }
  else
  {
    r = cursor->db_cursor->c_get (cursor->db_cursor, &dkey, &data, DB_NEXT);
    if (r == DB_NOTFOUND)
      return STATUS_OK;
    else if (r)
      return nerr_raise (NERR_DB, "Unable to get next item from cursor: %d", r);
  }

  /* allocate row */
  err = alloc_row (wdb, &my_row);
  if (err)
  {
    free (data.data);
    return nerr_pass(err);
  }

  my_row->key_value = (char *) malloc (dkey.size + 1);
  if (my_row->key_value == NULL)
  {
    free (data.data);
    free (my_row);
    return nerr_raise (NERR_NOMEM, "No memory for new row");
  }

  memcpy (my_row->key_value, dkey.data, dkey.size);
  my_row->key_value[dkey.size] = '\0';

  /* unpack row */
  err = unpack_row (wdb, data.data, data.size, my_row);
  free (data.data);
  free (dkey.data);
  if (err)
  {
    free (my_row);
    return nerr_pass(err);
  }

  *row = my_row;

  return STATUS_OK;
}

NEOERR *wdbr_find (WDB *wdb, WDBCursor *cursor, const char *key, WDBRow **row)
{
  DBT dkey, data;
  WDBRow *my_row;
  NEOERR *err = STATUS_OK;
  int r;

  *row = NULL;

  if (wdb->table_version != cursor->table_version)
  {
    return nerr_raise (NERR_ASSERT, "Cursor doesn't match database");
  }

  memset(&dkey, 0, sizeof(dkey));
  memset(&data, 0, sizeof(data));
  dkey.flags = DB_DBT_USERMEM;
  data.flags = DB_DBT_MALLOC;

  dkey.data = (void *)key;
  dkey.size = strlen(key);

  r = cursor->db_cursor->c_get (cursor->db_cursor, &dkey, &data, DB_SET_RANGE);
  if (r == DB_NOTFOUND)
    return STATUS_OK;
  else if (r)
    return nerr_raise (r, "Unable to get find item for key %s", key);

  /* allocate row */
  err = alloc_row (wdb, &my_row);
  if (err)
  {
    free (data.data);
    return nerr_pass(err);
  }

  my_row->key_value = (char *) malloc (dkey.size + 1);
  if (my_row->key_value == NULL)
  {
    free (data.data);
    free (my_row);
    return nerr_raise (NERR_NOMEM, "No memory for new row");
  }

  memcpy (my_row->key_value, dkey.data, dkey.size);
  my_row->key_value[dkey.size] = '\0';

  /* unpack row */
  err = unpack_row (wdb, data.data, data.size, my_row);
  free (data.data);
  if (err)
  {
    free (my_row);
    return nerr_pass(err);
  }

  *row = my_row;

  return STATUS_OK;
}

NEOERR *wdb_keys (WDB *wdb, char **primary_key, ULIST **data)
{
  NEOERR *err;
  int x, len;
  WDBColumn *col;
  ULIST *my_data;
  char *my_key = NULL;
  char *my_col = NULL;

  *data = NULL;
  *primary_key = NULL;
  my_key = strdup(wdb->key);
  if (my_key == NULL)
    return nerr_raise (NERR_NOMEM, "Unable to allocate memory for keys");

  len = uListLength(wdb->cols_l);
  err = uListInit (&my_data, len, 0);
  if (err != STATUS_OK) 
  {
    free(my_key);
    return nerr_pass(err);
  }

  for (x = 0; x < len; x++)
  {
    err = uListGet (wdb->cols_l, x, (void *)&col);
    if (err) goto key_err;
    my_col = strdup(col->name);
    if (my_col == NULL)
    {
      err = nerr_raise (NERR_NOMEM, "Unable to allocate memory for keys");
      goto key_err;
    }
    err = uListAppend (my_data, my_col);
    my_col = NULL;
    if (err) goto key_err;
  }

  *data = my_data;
  *primary_key = my_key;
  return STATUS_OK;

key_err:
  if (my_key != NULL) free (my_key);
  if (my_col != NULL) free (my_col);
  *primary_key = NULL;
  uListDestroy (&my_data, 0);
  return nerr_pass(err);
}

/*
 * Known Issues:
 *  - Probably need to store the actual key value in the packed row..
 *    Maybe not, because any cursor you use on a sleepycat db will
 *    return the key...
 * - um, memory.  Especially when dealing with rows, need to keep track
 *   of when we allocate, when we dealloc, and who owns that memory to
 *   free it
 * - function to delete entry from wdb
 */
