/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include "neo_err.h"
#include "neo_hdf.h"
#include "neo_str.h"
#include "ulist.h"

static NEOERR *_alloc_hdf (HDF **hdf, char *name, size_t nlen, char *value, 
    int dup, int wf, HDF *top)
{
  *hdf = calloc (1, sizeof (HDF));
  if (*hdf == NULL)
  {
    return nerr_raise (NERR_NOMEM, "Unable to allocate memory for hdf element");
  }

  (*hdf)->top = top;

  if (name != NULL)
  {
    (*hdf)->name_len = nlen;
    (*hdf)->name = (char *) malloc (nlen + 1);
    if ((*hdf)->name == NULL)
    {
      free((*hdf));
      (*hdf) = NULL;
      return nerr_raise (NERR_NOMEM, 
	  "Unable to allocate memory for hdf element: %s", name);
    }
    strncpy((*hdf)->name, name, nlen);
    (*hdf)->name[nlen] = '\0';
  }
  if (value != NULL)
  {
    if (dup)
    {
      (*hdf)->alloc_value = 1;
      (*hdf)->value = strdup(value);
      if ((*hdf)->value == NULL)
      {
	free((*hdf)->name);
	free((*hdf));
	(*hdf) = NULL;
	return nerr_raise (NERR_NOMEM, 
	    "Unable to allocate memory for hdf element %s", name);
      }
    }
    else
    {
      (*hdf)->alloc_value = wf;
      (*hdf)->value = value;
    }
  }
  return STATUS_OK;
}

static void _dealloc_hdf (HDF **hdf)
{
  if (*hdf == NULL) return;
  if ((*hdf)->child != NULL)
    _dealloc_hdf(&((*hdf)->child));
  if ((*hdf)->next != NULL)
    _dealloc_hdf(&((*hdf)->next));
  if ((*hdf)->name != NULL)
  {
    free ((*hdf)->name);
    (*hdf)->name = NULL;
  }
  if ((*hdf)->value != NULL)
  {
    if ((*hdf)->alloc_value)
      free ((*hdf)->value);
    (*hdf)->value = NULL;
  }
  free (*hdf);
  *hdf = NULL;
}

NEOERR* hdf_init (HDF **hdf)
{
  NEOERR *err;
  HDF *my_hdf;

  *hdf = NULL;

  err = nerr_init();
  if (err != STATUS_OK)
    return nerr_pass (err);

  err = _alloc_hdf (&my_hdf, NULL, 0, NULL, 0, 0, NULL);
  if (err != STATUS_OK)
    return nerr_pass (err);

  my_hdf->top = my_hdf;

  *hdf = my_hdf;

  return STATUS_OK;
}

void hdf_destroy (HDF **hdf)
{
  if (*hdf == NULL) return;
  if ((*hdf)->top == (*hdf))
    _dealloc_hdf(hdf);
}

static int _walk_hdf (HDF *hdf, char *name, HDF **node)
{
  HDF *hp = hdf;
  int x = 0;
  char *s = name;
  char *n = name;
  int r;

  *node = NULL;

  if (hdf == NULL) return -1;
  if (name == NULL || name[0] == '\0')
  {
    *node = hdf;
    return 0;
  }

  if (hdf->link)
  {
    r = _walk_hdf (hdf->top, hdf->value, &hp);
    if (r) return r;
    if (hp) hp = hp->child;
  }
  else
  {
    hp = hdf->child;
  }
  if (hp == NULL)
  {
    return -1;
  }

  n = name;
  s = strchr (n, '.');
  x = (s == NULL) ? strlen(n) : s - n;

  while (1)
  {
    while (hp != NULL)
    {
      if (hp->name && (x == hp->name_len) && !strncmp(hp->name, n, x))
      {
	break;
      }
      else
      {
	hp = hp->next;
      }
    }
    if (hp == NULL)
    {
      return -1;
    }
    if (s == NULL) break;
   
    if (hp->link)
    {
      r = _walk_hdf (hp->top, hp->value, &hp);
      if (r) return r;
      hp = hp->child;
    }
    else
    {
      hp = hp->child;
    }
    n = s + 1;
    s = strchr (n, '.');
    x = (s == NULL) ? strlen(n) : s - n;
  } 
  if (hp->link)
    return _walk_hdf (hp->top, hp->value, node);

  *node = hp;
  return 0;
}

int hdf_get_int_value (HDF *hdf, char *name, int defval)
{
  HDF *node;
  int v;
  char *n;

  if ((_walk_hdf(hdf, name, &node) == 0) && (node->value != NULL))
  {
    v = strtol (node->value, &n, 10);
    if (node->value == n) v = defval;
    return v;
  }
  return defval;
}

char* hdf_get_value (HDF *hdf, char *name, char *defval)
{
  HDF *node;

  if ((_walk_hdf(hdf, name, &node) == 0) && (node->value != NULL))
  {
    return node->value;
  }
  return defval;
}

NEOERR* hdf_get_copy (HDF *hdf, char *name, char **value, char *defval)
{
  HDF *node;

  if ((_walk_hdf(hdf, name, &node) == 0) && (node->value != NULL))
  {
    *value = strdup(node->value);
    if (*value == NULL)
    {
      return nerr_raise (NERR_NOMEM, "Unable to allocate copy of %s", name);
    }
  }
  else
  {
    if (defval == NULL)
      *value = NULL;
    else
    {
      *value = strdup(defval);
      if (*value == NULL)
      {
	return nerr_raise (NERR_NOMEM, "Unable to allocate copy of %s", name);
      }
    }
  }
  return STATUS_OK;
}

HDF* hdf_get_obj (HDF *hdf, char *name)
{
  HDF *obj;

  _walk_hdf(hdf, name, &obj);
  return obj;
}

HDF* hdf_get_child (HDF *hdf, char *name)
{
  HDF *obj;
  _walk_hdf(hdf, name, &obj);
  if (obj != NULL) return obj->child;
  return obj;
}

HDF* hdf_obj_child (HDF *hdf)
{
  HDF *obj;
  if (hdf == NULL) return NULL;
  if (hdf->link)
  {
    if (_walk_hdf(hdf->top, hdf->value, &obj))
      return NULL;
    return obj->child;
  }
  return hdf->child;
}

HDF* hdf_obj_next (HDF *hdf)
{
  if (hdf == NULL) return NULL;
  return hdf->next;
}

char* hdf_obj_name (HDF *hdf)
{
  if (hdf == NULL) return NULL;
  return hdf->name;
}

char* hdf_obj_value (HDF *hdf)
{
  int count = 0;

  if (hdf == NULL) return NULL;
  while (hdf->link && count < 100)
  {
    if (_walk_hdf (hdf->top, hdf->value, &hdf))
      return NULL;
    count++;
  }
  return hdf->value;
}

NEOERR* _set_value (HDF *hdf, char *name, char *value, int dup, int wf, int link)
{
  NEOERR *err;
  HDF *hn, *hp, *hs;
  int x = 0;
  char *s = name;
  char *n = name;

  if (hdf == NULL)
  {
    return nerr_raise(NERR_ASSERT, "Unable to set %s on NULL hdf", name);
  }

  /* HACK: allow setting of this node by passing an empty name */
  if (name == NULL || name[0] == '\0')
  {
    if (hdf->alloc_value)
    {
      free(hdf->value);
      hdf->value = NULL;
    }
    if (dup)
    {
      hdf->alloc_value = 1;
      hdf->value = strdup(value);
      if (hdf->value == NULL)
	return nerr_raise (NERR_NOMEM, "Unable to duplicate value %s for %s", 
	    value, name);
    }
    else
    {
      hdf->alloc_value = wf;
      hdf->value = value;
    }
    return STATUS_OK;
  }

  n = name;
  s = strchr (n, '.');
  x = (s != NULL) ? s - n : strlen(n);
  if (x == 0)
  {
    return nerr_raise(NERR_ASSERT, "Unable to set Empty component %s", name);
  }

  hn = hdf;

  while (1)
  {
    hp = hn->child;
    hs = NULL;

    while (hp != NULL)
    {
      if (hp->name && (x == hp->name_len) && !strncmp(hp->name, n, x))
      {
	break;
      }
      hs = hp;
      hp = hp->next;
    }
    if (hp == NULL)
    {
      if (s != NULL)
      {
	err = _alloc_hdf (&hp, n, x, NULL, 0, 0, hdf->top);
      }
      else
      {
	err = _alloc_hdf (&hp, n, x, value, dup, wf, hdf->top);
	if (link) hp->link = 1;
      }
      if (err != STATUS_OK)
	return nerr_pass (err);
      if (hn->child == NULL)
	hn->child = hp;
      else
	hs->next = hp;
    }
    else if (s == NULL)
    {
      if (hp->alloc_value)
      {
	free(hp->value);
	hp->value = NULL;
      }
      if (dup)
      {
	hp->alloc_value = 1;
	hp->value = strdup(value);
	if (hp->value == NULL)
	  return nerr_raise (NERR_NOMEM, "Unable to duplicate value %s for %s", 
	      value, name);
      }
      else
      {
	hp->alloc_value = wf;
	hp->value = value;
      }
      if (link) hp->link = 1;
    }
    if (s == NULL)
      break;
    n = s + 1;
    s = strchr (n, '.');
    x = (s != NULL) ? s - n : strlen(n);
    if (x == 0)
    {
      return nerr_raise(NERR_ASSERT, "Unable to set Empty component %s", name);
    }
    hn = hp;
  }
  return STATUS_OK;
}

NEOERR* hdf_set_value (HDF *hdf, char *name, char *value)
{
  return nerr_pass(_set_value (hdf, name, value, 1, 1, 0));
}

NEOERR* hdf_set_symlink (HDF *hdf, char *src, char *dest)
{
  return nerr_pass(_set_value (hdf, src, dest, 1, 1, 1));
}

NEOERR* hdf_set_int_value (HDF *hdf, char *name, int value)
{
  char buf[256];

  snprintf (buf, sizeof(buf), "%d", value);
  return nerr_pass(_set_value (hdf, name, buf, 1, 1, 0));
}

NEOERR* hdf_set_buf (HDF *hdf, char *name, char *value)
{
  return nerr_pass(_set_value (hdf, name, value, 0, 1, 0));
}

NEOERR* hdf_set_copy (HDF *hdf, char *dest, char *src)
{
  HDF *node;
  if ((_walk_hdf(hdf, src, &node) == 0) && (node->value != NULL))
  {
    return nerr_pass(_set_value (hdf, dest, node->value, 0, 0, 0));
  }
  return nerr_raise (NERR_NOT_FOUND, "Unable to find %s", src);
}

/* grody bubble sort from Wheeler, but sorting a singly linked list
 * is annoying */
void hdf_sort_obj_bubble(HDF *h, int (*compareFunc)(HDF *, HDF *))
{
  HDF *p, *c = hdf_obj_child(h);
  HDF *prev;
  int swapped;

  do {
    swapped = 0;
    for (p = c, prev = c; p && p->next; prev = p, p = p->next) {
      if (compareFunc(p, p->next) > 0) {
	HDF *tmp = p->next;
	prev->next = tmp;
	p->next = tmp->next;
	tmp->next = p;
	p = tmp;
	swapped = 1;
      }
    }
  } while (swapped);
}

/* Ok, this version avoids the bubble sort by walking the level once to
 * load them all into a ULIST, qsort'ing the list, and then dumping them
 * back out... */
NEOERR *hdf_sort_obj (HDF *h, int (*compareFunc)(const void *, const void *))
{
  NEOERR *err = STATUS_OK;
  ULIST *level = NULL;
  HDF *p, *c;
  int x;

  if (h == NULL) return STATUS_OK;
  c = h->child;
  if (c == NULL) return STATUS_OK;

  do {
    err = uListInit(&level, 40, 0);
    if (err) return nerr_pass(err);
    for (p = c; p; p = p->next) {
      err = uListAppend(level, p);
      if (err) break;
    }
    err = uListSort(level, compareFunc);
    if (err) break;
    uListGet(level, 0, (void **)&c);
    h->child = c;
    for (x = 1; x < uListLength(level); x++)
    {
      uListGet(level, x, (void **)&p);
      c->next = p;
      p->next = NULL;
      c = p;
    }
  } while (0);
  uListDestroy(&level, 0);
  return nerr_pass(err);
}

NEOERR* hdf_remove_tree (HDF *hdf, char *name)
{
  HDF *hp = hdf;
  HDF *lp = NULL, *ln = NULL;
  int x = 0;
  char *s = name;
  char *n = name;

  if (hdf == NULL) return STATUS_OK;

  hp = hdf->child;
  if (hp == NULL)
  {
    return STATUS_OK;
  }

  n = name;
  s = strchr (n, '.');
  x = (s == NULL) ? strlen(n) : s - n;

  while (1)
  {
    while (hp != NULL)
    {
      if (hp->name && (x == hp->name_len) && !strncmp(hp->name, n, x))
      {
	break;
      }
      else
      {
	lp = NULL;
	ln = hp;
	hp = hp->next;
      }
    }
    if (hp == NULL)
    {
      return STATUS_OK;
    }
    if (s == NULL) break;
   
    lp = hp;
    ln = NULL;
    hp = hp->child;
    n = s + 1;
    s = strchr (n, '.');
    x = (s == NULL) ? strlen(n) : s - n;
  } 

  if (lp)
  {
    lp->child = hp->next;
    hp->next = NULL;
  }
  else if (ln)
  {
    ln->next = hp->next;
    hp->next = NULL;
  }
  _dealloc_hdf (&hp);

  return STATUS_OK;
}

static NEOERR * _copy_nodes (HDF *dest, HDF *src)
{
  NEOERR *err = STATUS_OK;
  HDF *dt, *st;
  BOOL alloc;

  st = src->child;
  while (st != NULL)
  {
    dt = dest->child;
    if (dt == NULL)
    {
      err = _alloc_hdf (&(dest->child), st->name, st->name_len, st->value, 1, 0, dest->top);
      dt = dest->child;
    }
    else 
    {
      alloc = FALSE;
      while (dt != NULL)
      {
	if (strcmp (dt->name, st->name))
	{
	  if (dt->next == NULL) break;
	  dt = dt->next;
	}
	else
	{
	  alloc = TRUE;
	  if (st->value != NULL)
	  {
	    if (dt->alloc_value)
	    {
	      free(dt->value);
	      dt->value = NULL;
	    }
	    dt->alloc_value = 1;
	    dt->value = strdup(st->value);
	    if (dt->value == NULL)
	      return nerr_raise (NERR_NOMEM, "Unable to copy value %s for %s", 
		  st->value, st->name);
	  }
	  break;
	}
      }
      if (alloc == FALSE)
      {
	err = _alloc_hdf (&(dt->next), st->name, st->name_len, st->value, 1, 0, dest->top);
	dt = dt->next;
      }
    }
    if (err) return nerr_pass(err);
    err = _copy_nodes (dt, st);
    if (err) return nerr_pass(err);
    st = st->next;
  }
  return STATUS_OK;
}

NEOERR* hdf_copy (HDF *dest, char *name, HDF *src)
{
  NEOERR *err;
  HDF *node;

  if (_walk_hdf(dest, name, &node) == -1)
  {
    err = _set_value (dest, name, NULL, 0, 0, 0);
    if (err) return nerr_pass (err);
    if (_walk_hdf(dest, name, &node) == -1)
      return nerr_raise(NERR_ASSERT, "Um, this shouldn't happen");
  }
  return nerr_pass (_copy_nodes (node, src));
}

NEOERR* hdf_dump(HDF *hdf, char *prefix)
{
  char *p;
  char op = '=';

  if (hdf->value)
  {
    if (hdf->link) op = ':';
    if (prefix)
    {
      printf("%s.%s %c %s\n", prefix, hdf->name, op, hdf->value);
    }
    else
    {
      printf("%s %c %s\n", hdf->name, op, hdf->value);
    }
  }
  if (hdf->child)
  {
    if (prefix)
    {
      p = (char *) malloc (strlen(hdf->name) + strlen(prefix) + 2);
      sprintf (p, "%s.%s", prefix, hdf->name);
      hdf_dump(hdf->child, p);
      free(p);
    }
    else
    {
      hdf_dump(hdf->child, hdf->name);
    }
  }
  if (hdf->next)
  {
    hdf_dump(hdf->next, prefix);
  }
  return STATUS_OK;
}

NEOERR* hdf_dump_str(HDF *hdf, char *prefix, int compact, STRING *str)
{
  NEOERR *err;
  char *p;
  char op = '=';

  if (hdf->value)
  {
    if (hdf->link) op = ':';
    if (prefix && !compact)
    {
      err = string_appendf (str, "%s.%s", prefix, hdf->name);
    }
    else
    {
      err = string_append (str, hdf->name);
    }
    if (err) return nerr_pass (err);
    if (strchr (hdf->value, '\n'))
    {
      if (hdf->value[strlen(hdf->value)-1] != '\n')
	err = string_appendf (str, " << EOM\n%s\nEOM\n", hdf->value);
      else
	err = string_appendf (str, " << EOM\n%sEOM\n", hdf->value);
    }
    else
    {
      err = string_appendf (str, " %c %s\n", op, hdf->value);
    }
    if (err) return nerr_pass (err);
  }
  if (hdf->child)
  {
    if (prefix && !compact)
    {
      p = (char *) malloc (strlen(hdf->name) + strlen(prefix) + 2);
      sprintf (p, "%s.%s", prefix, hdf->name);
      err = hdf_dump_str (hdf->child, p, compact, str);
      free(p);
    }
    else
    {
      if (compact && hdf->name)
      {
	err = string_appendf(str, "%s {\n", hdf->name);
	if (err) return nerr_pass (err);
	err = hdf_dump_str (hdf->child, hdf->name, compact, str);
	if (err) return nerr_pass (err);
	err = string_append(str, "}\n");
      }
      else
      {
	err = hdf_dump_str (hdf->child, hdf->name, compact, str);
      }
    }
    if (err) return nerr_pass (err);
  }
  if (hdf->next)
  {
    err = hdf_dump_str (hdf->next, prefix, compact, str);
    if (err) return nerr_pass (err);
  }
  return STATUS_OK;
}

NEOERR* hdf_dump_format (HDF *hdf, int lvl, FILE *fp)
{
  char prefix[256];
  char op = '=';

  memset(prefix, ' ', 256);
  if (lvl > 127)
    lvl = 127;
  prefix[lvl*2] = '\0';

  if (hdf->value)
  {
    if (hdf->link) op = ':';
    if (strchr (hdf->value, '\n'))
    {
      if (hdf->value[strlen(hdf->value)-1] != '\n')
	fprintf(fp, "%s%s << EOM\n%s\nEOM\n", prefix, hdf->name, hdf->value);
      else
	fprintf(fp, "%s%s << EOM\n%sEOM\n", prefix, hdf->name, hdf->value);
    }
    else
    {
      fprintf(fp, "%s%s %c %s\n", prefix, hdf->name, op, hdf->value);
    }
  }
  if (hdf->child)
  {
    if (hdf->name)
    {
      fprintf(fp, "%s%s {\n", prefix, hdf->name);
      hdf_dump_format(hdf->child, lvl+1, fp);
      fprintf(fp, "%s}\n", prefix);
    }
    else
    {
      hdf_dump_format(hdf->child, lvl, fp);
    }
  }
  if (hdf->next)
  {
    hdf_dump_format(hdf->next, lvl, fp);
  }
  return STATUS_OK;
}

NEOERR *hdf_write_file (HDF *hdf, char *path)
{
  FILE *fp;

  fp = fopen(path, "w");
  if (fp == NULL)
    return nerr_raise_errno (NERR_IO, "Unable to open %s for writing", path);

  hdf_dump_format (hdf, 0, fp);

  fclose (fp);
  return STATUS_OK;
}

NEOERR *hdf_write_string (HDF *hdf, char **s)
{
  STRING str;
  NEOERR *err;

  *s = NULL;

  string_init (&str);

  err = hdf_dump_str (hdf, NULL, 1, &str);
  if (err) 
  {
    string_clear (&str);
    return nerr_pass(err);
  }

  *s = str.buf;

  return STATUS_OK;
}


/* HDF file looks like the following: */
#define SKIPWS(s) while (*s && isspace(*s)) s++;

static int _copy_line (char **s, char *buf, size_t buf_len)
{
  int x = 0;
  char *st = *s;

  while (*st && x < buf_len)
  {
    buf[x++] = *st;
    if (*st++ == '\n') break;
  }
  buf[x] = '\0';
  *s = st;

  return x;
}

static NEOERR* _hdf_read_string (HDF *hdf, char **str, int *line)
{
  NEOERR *err;
  HDF *lower;
  char buf[4096];
  char *s;
  char *name, *value;

  while (_copy_line(str, buf, sizeof(buf)) != 0)
  {
    (*line)++;
    s = buf;
    SKIPWS(s);
    if (!strncmp(s, "#include ", 9))
    {
      return nerr_raise (NERR_PARSE, "[%d]: #include not supported in string parse", *line);
    }
    else if (s[0] == '#')
    {
      /* comment: pass */
    }
    else if (s[0] == '}') /* up */
    {
      s = neos_strip(s);
      if (strcmp(s, "}"))
      {
	return nerr_raise(NERR_PARSE, 
	    "[%d] Trailing garbage on line following }: %s", *line,
	    buf);
      }
      return STATUS_OK;
    }
    else if (s[0])
    {
      /* Valid hdf name is [0-9a-zA-Z_.]+ */
      name = s;
      while (*s && (isalnum(*s) || *s == '_' || *s == '.')) s++;
      /*
      if (*s != '\0')
      {
	*s++ = '\0';
      }
      */
      SKIPWS(s);

      if (s[0] == '=') /* assignment */
      {
	*s = '\0';
	name = neos_strip(name);
	s++;
	value = neos_strip(s);
	err = hdf_set_value (hdf, name, value);
	if (err != STATUS_OK)
	  return nerr_pass_ctx(err, "In String %d", *line);
      }
      else if (s[0] == ':') /* link */
      {
	*s = '\0';
	name = neos_strip(name);
	s++;
	value = neos_strip(s);
	err = hdf_set_symlink (hdf, name, value);
	if (err != STATUS_OK)
	  return nerr_pass_ctx(err, "In string %d", *line);
      }
      else if (s[0] == '{') /* deeper */
      {
	*s = '\0';
	name = neos_strip(name);
	lower = hdf_get_obj (hdf, name);
	if (lower == NULL)
	{
	  err = hdf_set_value (hdf, name, NULL);
	  if (err != STATUS_OK) 
	    return nerr_pass_ctx(err, "In string %d", *line);
	  lower = hdf_get_obj (hdf, name);
	}
	err = _hdf_read_string (lower, str, line);
	if (err != STATUS_OK) 
	  return nerr_pass_ctx(err, "In string %d", *line);
      }
      else if (s[0] == '<' && s[1] == '<') /* multi-line assignment */
      {
	char *m;
	int msize = 0;
	int mmax = 128;
	int l;

	*s = '\0';
	name = neos_strip(name);
	s+=2;
	value = neos_strip(s);
	l = strlen(value);
	if (l == 0)
	  return nerr_raise(NERR_PARSE, 
	      "[%d] No multi-assignment terminator given: %s", *line, 
	      buf);
	m = (char *) malloc (mmax * sizeof(char));
	if (m == NULL)
	  return nerr_raise(NERR_NOMEM, 
	    "[%d] Unable to allocate memory for multi-line assignment to %s",
	    *line, name);
	while (_copy_line (str, m+msize, mmax-msize) != 0)
	{
	  if (!strncmp(value, m+msize, l) && isspace(m[msize+l]))
	  {
	    m[msize] = '\0';
	    break;
	  }
	  msize += strlen(m+msize);
	  if (msize + l + 10 > mmax)
	  {
	    mmax += 128;
	    m = (char *) realloc (m, mmax * sizeof(char));
	    if (m == NULL)
	      return nerr_raise(NERR_NOMEM, 
		  "[%d] Unable to allocate memory for multi-line assignment to %s: size=%d",
		  *line, name, mmax);
	  }
	}
	err = hdf_set_buf(hdf, name, m);
	if (err != STATUS_OK)
	{
	  free (m);
	  return nerr_pass_ctx(err, "In string %d", *line);
	}

      }
      else
      {
	return nerr_raise(NERR_PARSE, "[%d] Unable to parse line %s",
	    *line, buf);
      }
    }
  }
  return STATUS_OK;
}

NEOERR * hdf_read_string (HDF *hdf, char *str)
{
  int line = 0;
  return nerr_pass (_hdf_read_string (hdf, &str, &line));
}

static int count_newlines (char *s)
{
  int i = 0;
  char *n = s;

  n = strchr(s, '\n');
  while (n != NULL)
  {
    i++;
    n = strchr(n+1, '\n');
  }
  return i;
}

static NEOERR* hdf_read_file_fp (HDF *hdf, FILE *fp, char *path, int *line)
{
  NEOERR *err;
  STRING str;
  HDF *lower;
  char buf[4096];
  char *s;
  char *name, *value;
  int l;

  string_init(&str);
  err = string_readline(&str, fp);
  if (err) return nerr_pass(err);
  while (str.len != 0)
  {
    (*line)++;
    s = str.buf;
    SKIPWS(s);
    if (!strncmp(s, "#include ", 9))
    {
      s += 9;
      name = neos_strip(s);
      l = strlen(name);
      if (name[0] == '"' && name[l-1] == '"')
      {
	name[l-1] = '\0';
	name++;
      }
      err = hdf_read_file(hdf, name);
      if (err != STATUS_OK) 
	return nerr_pass_ctx(err, "In file %s:%d", path, *line);
    }
    else if (s[0] == '#')
    {
      /* comment: pass */
    }
    else if (s[0] == '}') /* up */
    {
      s = neos_strip(s);
      if (strcmp(s, "}"))
      {
	return nerr_raise(NERR_PARSE, 
	    "[%s:%d] Trailing garbage on line following }: %s", path, *line,
	    buf);
      }
      return STATUS_OK;
    }
    else if (s[0])
    {
      /* Valid hdf name is [0-9a-zA-Z_.]+ */
      name = s;
      while (*s && (isalnum(*s) || *s == '_' || *s == '.')) s++;
      /*
      if (*s != '\0')
      {
	*s++ = '\0';
      }
      */
      SKIPWS(s);

      if (s[0] == '=') /* assignment */
      {
	*s = '\0';
	name = neos_strip(name);
	s++;
	value = neos_strip(s);
	err = hdf_set_value (hdf, name, value);
	if (err != STATUS_OK)
	  return nerr_pass_ctx(err, "In file %s:%d", path, *line);
      }
      else if (s[0] == ':') /* link */
      {
	*s = '\0';
	name = neos_strip(name);
	s++;
	value = neos_strip(s);
	err = hdf_set_symlink (hdf, name, value);
	if (err != STATUS_OK)
	  return nerr_pass_ctx(err, "In file %s:%d", path, *line);
      }
      else if (s[0] == '{') /* deeper */
      {
	*s = '\0';
	name = neos_strip(name);
	lower = hdf_get_obj (hdf, name);
	if (lower == NULL)
	{
	  err = hdf_set_value (hdf, name, NULL);
	  if (err != STATUS_OK) 
	    return nerr_pass_ctx(err, "In file %s:%d", path, *line);
	  lower = hdf_get_obj (hdf, name);
	}
	err = hdf_read_file_fp(lower, fp, path, line);
	if (err != STATUS_OK) 
	  return nerr_pass_ctx(err, "In file %s:%d", path, *line);
	if (feof(fp)) break;
      }
      else if (s[0] == '<' && s[1] == '<') /* multi-line assignment */
      {
	char *m;
	int msize = 0;
	int mmax = 128;
	int l;

	*s = '\0';
	name = neos_strip(name);
	s+=2;
	value = neos_strip(s);
	l = strlen(value);
	if (l == 0)
	  return nerr_raise(NERR_PARSE, 
	      "[%s:%d] No multi-assignment terminator given: %s", path, *line, 
	      buf);
	m = (char *) malloc (mmax * sizeof(char));
	if (m == NULL)
	  return nerr_raise(NERR_NOMEM, 
	    "[%s:%d] Unable to allocate memory for multi-line assignment to %s",
	    path, *line, name);
	while (fgets(m+msize, mmax-msize, fp) != NULL)
	{
	  if (!strncmp(value, m+msize, l) && (m[msize+l] == '\r' || m[msize+l] == '\n'))
	  {
	    m[msize] = '\0';
	    break;
	  }
	  msize += strlen(m+msize);
	  if (msize + l + 10 > mmax)
	  {
	    mmax += 128;
	    m = (char *) realloc (m, mmax * sizeof(char));
	    if (m == NULL)
	      return nerr_raise(NERR_NOMEM, 
		  "[%s:%d] Unable to allocate memory for multi-line assignment to %s: size=%d",
		  path, *line, name, mmax);
	  }
	}
	err = hdf_set_buf(hdf, name, m);
	if (err != STATUS_OK)
	{
	  free (m);
	  return nerr_pass_ctx(err, "In file %s:%d", path, *line);
	}
	/* count the newlines so we know what line we're on... +1 for EOM */
	(*line) = (*line) + count_newlines(m) + 1;
      }
      else
      {
	return nerr_raise(NERR_PARSE, "[%s:%d] Unable to parse line %s",
	    path, *line, buf);
      }
    }
    str.len = 0;
    err = string_readline(&str, fp);
    if (err) return nerr_pass(err);
  }
  return STATUS_OK;
}

/* The search path is part of the HDF by convention */
NEOERR* hdf_search_path (HDF *hdf, char *path, char *full)
{
  HDF *paths;
  struct stat s;

  for (paths = hdf_get_child (hdf, "hdf.loadpaths");
      paths;
      paths = hdf_obj_next (paths))
  {
    snprintf (full, _POSIX_PATH_MAX, "%s/%s", hdf_obj_value(paths), path);
    errno = 0;
    if (stat (full, &s) == -1)
    {
      if (errno != ENOENT)
	return nerr_raise_errno (NERR_SYSTEM, "Stat of %s failed", full);
    }
    else 
    {
      return STATUS_OK;
    }
  }

  strncpy (full, path, _POSIX_PATH_MAX);
  if (stat (full, &s) == -1)
  {
    if (errno != ENOENT)
      return nerr_raise_errno (NERR_SYSTEM, "Stat of %s failed", full);
  }
  else return STATUS_OK;

  return nerr_raise (NERR_NOT_FOUND, "Path %s not found", path);
}

NEOERR* hdf_read_file (HDF *hdf, char *path)
{
  NEOERR *err;
  FILE *fp;
  int line = 0;
  char fpath[_POSIX_PATH_MAX];

  if (path == NULL) 
    return nerr_raise(NERR_ASSERT, "Can't read NULL file");
  if (path[0] != '/')
  {
    err = hdf_search_path (hdf, path, fpath);
    if (err != STATUS_OK) return nerr_pass(err);
    path = fpath;
  }

  fp = fopen(path, "r");
  if (fp == NULL)
  {
    if (errno == ENOENT)
    {
      return nerr_raise(NERR_NOT_FOUND, "File not found: %s", path);
    }
    else
    {
      return nerr_raise_errno(NERR_IO, "Unable to open file %s", path);
    }
  }

  err = hdf_read_file_fp(hdf, fp, path, &line);
  fclose(fp);
  return nerr_pass(err);
}

