/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the FSF's 
 * Library Gnu Public License (LGPL). 
 *
 * Copyright (C) 2001 by Brandon Long 
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include "util/neo_err.h"
#include "util/neo_misc.h"
#include "util/neo_str.h"
#include "util/ulist.h"
#include "cs.h"

typedef enum
{
  ST_SAME = 0,
  ST_GLOBAL = 1<<0,
  ST_IF = 1<<1,
  ST_ELSE = 1<<2,
  ST_EACH = 1<<3,
  ST_POP = 1<<4,
  ST_DEF = 1<<5,
} CS_STATE;

#define ST_ANYWHERE (ST_EACH | ST_ELSE | ST_IF | ST_GLOBAL | ST_DEF)

typedef struct _stack_entry 
{
  int state;
  CSTREE *tree;
  CSTREE *next_tree;
  int num_local;
  int location;
} STACK_ENTRY;

static NEOERR *literal_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *literal_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *name_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *name_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *var_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *var_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *evar_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *if_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *if_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *else_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *elif_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *endif_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *each_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *each_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *endeach_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *include_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *def_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *skip_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *enddef_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *call_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *call_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *set_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *set_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);

static NEOERR *render_node (CSPARSE *parse, CSTREE *node);

typedef struct _cmds
{
  char *cmd;
  int cmdlen;
  int allowed_state;
  int next_state;
  NEOERR* (*parse_handler)(CSPARSE *parse, int cmd, char *arg);
  NEOERR* (*eval_handler)(CSPARSE *parse, CSTREE *node, CSTREE **next);
  int has_arg;
} CS_CMDS;

CS_CMDS Commands[] = {
  {"literal", sizeof("literal")-1, ST_ANYWHERE,     ST_SAME, 
    literal_parse, literal_eval, 0},
  {"name",     sizeof("name")-1,     ST_ANYWHERE,     ST_SAME, 
    name_parse, name_eval,     1},
  {"var",     sizeof("var")-1,     ST_ANYWHERE,     ST_SAME, 
    var_parse, var_eval,     1},
  {"evar",    sizeof("evar")-1,    ST_ANYWHERE,     ST_SAME, 
    evar_parse, skip_eval,    1},
  {"if",      sizeof("if")-1,      ST_ANYWHERE,     ST_IF,   
    if_parse, if_eval,      1},
  {"else",    sizeof("else")-1,    ST_IF,           ST_POP | ST_ELSE, 
    else_parse, skip_eval,    0},
  {"elseif",  sizeof("elseif")-1,  ST_IF,           ST_SAME, 
    elif_parse, if_eval,   1},
  {"elif",    sizeof("elif")-1,    ST_IF,           ST_SAME, 
    elif_parse, if_eval,   1},
  {"/if",     sizeof("/if")-1,     ST_IF | ST_ELSE, ST_POP,  
    endif_parse, skip_eval,   0},
  {"each",    sizeof("each")-1,    ST_ANYWHERE,     ST_EACH, 
    each_parse, each_eval,    1},
  {"/each",   sizeof("/each")-1,   ST_EACH,         ST_POP,  
    endeach_parse, skip_eval, 0},
  {"include", sizeof("include")-1, ST_ANYWHERE,     ST_SAME, 
    include_parse, skip_eval, 1},
  {"def",     sizeof("def")-1,     ST_GLOBAL,       ST_DEF, 
    def_parse, skip_eval, 1},
  {"/def",    sizeof("/def")-1,    ST_DEF,          ST_POP,
    enddef_parse, skip_eval, 0},
  {"call",    sizeof("call")-1,    ST_ANYWHERE,     ST_SAME,
    call_parse, call_eval, 1},
  {"set",    sizeof("set")-1,    ST_ANYWHERE,     ST_SAME,
    set_parse, set_eval, 1},
  {NULL},
};

static int NodeNumber = 0;

static NEOERR *alloc_node (CSTREE **node)
{
  CSTREE *my_node;

  *node = NULL;
  my_node = (CSTREE *) calloc (1, sizeof (CSTREE));
  if (my_node == NULL)
    return nerr_raise (NERR_NOMEM, "Unable to allocate memory for node");

  my_node->cmd = 0;
  my_node->node_num = NodeNumber++;

  *node = my_node;
  return STATUS_OK;
}

static void dealloc_node (CSTREE **node)
{
  CSTREE *my_node;
  CSARG *arg, *p;

  if (*node == NULL) return;
  my_node = *node;
  if (my_node->case_0) dealloc_node (&(my_node->case_0));
  if (my_node->case_1) dealloc_node (&(my_node->case_1));
  if (my_node->next) dealloc_node (&(my_node->next));
  if (my_node->vargs)
  {
    arg = my_node->vargs;
    while (arg)
    {
      p = arg->next;
      free(arg);
      arg = p;
    }
  }
  free(my_node);
  *node = NULL;
}

static void dealloc_macro (CS_MACRO **macro)
{
  CS_MACRO *my_macro;
  CSARG *arg, *p;

  if (*macro == NULL) return;
  my_macro = *macro;
  if (my_macro->name) free (my_macro->name);
  if (my_macro->args)
  {
    arg = my_macro->args;
    while (arg)
    {
      p = arg->next;
      free(arg);
      arg = p;
    }
  }
  if (my_macro->next) dealloc_macro (&(my_macro->next));
  free (my_macro);
  *macro = NULL;
}

NEOERR *cs_init (CSPARSE **parse, HDF *hdf)
{
  NEOERR *err = STATUS_OK;
  CSPARSE *my_parse;
  STACK_ENTRY *entry;

  err = nerr_init();
  if (err != STATUS_OK) return nerr_pass (err);

  my_parse = (CSPARSE *) calloc (1, sizeof (CSPARSE));
  if (my_parse == NULL)
    return nerr_raise (NERR_NOMEM, "Unable to allocate memory for CSPARSE");


  err = uListInit (&(my_parse->stack), 10, 0);
  if (err != STATUS_OK)
  {
    free(my_parse);
    return nerr_pass(err);
  }
  err = alloc_node (&(my_parse->tree));
  if (err != STATUS_OK)
  {
    cs_destroy (&my_parse);
    return nerr_pass(err);
  }
  my_parse->current = my_parse->tree;
  my_parse->next = &(my_parse->current->next);

  entry = (STACK_ENTRY *) calloc (1, sizeof (STACK_ENTRY));
  if (entry == NULL)
  {
    cs_destroy (&my_parse);
    return nerr_raise (NERR_NOMEM, 
	"Unable to allocate memory for stack entry");
  }
  entry->state = ST_GLOBAL;
  entry->tree = my_parse->current;
  entry->location = 0;
  err = uListAppend(my_parse->stack, entry);
  if (err != STATUS_OK) {
    free (entry);
    cs_destroy(&my_parse);
    return nerr_pass(err);
  }
  my_parse->hdf = hdf;

  *parse = my_parse;
  return STATUS_OK;
}

void cs_destroy (CSPARSE **parse)
{
  CSPARSE *my_parse = *parse;

  if (my_parse == NULL)
    return;

  uListDestroy (&(my_parse->stack), ULIST_FREE);

  dealloc_macro(&my_parse->macros);
  dealloc_node(&(my_parse->tree));

  free(my_parse);
  *parse = NULL;
}

static int find_open_delim (char *buf, int x, int len)
{
  char *p;

  while (x < len)
  {
    p = strchr (&(buf[x]), '<');
    if (p == NULL) return -1;
    if (p[1] && p[1] == '?' &&
	p[2] && (p[2] == 'C' || p[2] == 'c') &&
	p[3] && (p[3] == 'S' || p[3] == 's') &&
	p[4] && p[4] == ' ')
    {
      return p - buf;
    }
    x = p - buf + 1;
  }
  return -1;
}

NEOERR *cs_parse_file (CSPARSE *parse, char *path)
{
  NEOERR *err;
  struct stat s;
  int fd;
  char *ibuf;
  size_t ibuf_len;
  char *save_context;
  int save_infile;
  char fpath[_POSIX_PATH_MAX];

  if (path[0] != '/')
  {
    err = hdf_search_path (parse->hdf, path, fpath);
    if (err != STATUS_OK) return nerr_pass(err);
    path = fpath;
  }

  if (stat(path, &s) == -1)
  {
    return nerr_raise (NERR_SYSTEM, "Unable to stat file %s: [%d] %s", path, 
	errno, strerror(errno));
  }

  fd = open (path, O_RDONLY);
  if (fd == -1)
  {
    return nerr_raise (NERR_SYSTEM, "Unable to open file %s: [%d] %s", path, 
	errno, strerror(errno));
  }
  ibuf_len = s.st_size;
  ibuf = (char *) malloc (ibuf_len + 1);

  if (ibuf == NULL)
  {
    close(fd);
    return nerr_raise (NERR_NOMEM, 
	"Unable to allocate memory (%d) to load file %s", s.st_size, path);
  }
  if (read (fd, ibuf, ibuf_len) == -1)
  {
    close(fd);
    free(ibuf);
    return nerr_raise (NERR_SYSTEM, "Unable to read file %s: [%d] %s", path, 
	errno, strerror(errno));
  }
  ibuf[ibuf_len] = '\0';
  close(fd);

  save_context = parse->context;
  parse->context = path;
  save_infile = parse->in_file;
  parse->in_file = 1;
  err = cs_parse_string(parse, ibuf, ibuf_len);
  parse->in_file = save_infile;
  parse->context = save_context;

  return nerr_pass(err);
}

static char *find_context (CSPARSE *parse, int offset, char *buf, size_t blen)
{
  FILE *fp;
  int err = 1;
  char *context;
  char line[256];
  int count = 0;
  int lineno = 0;

  context = parse->context ? parse->context : "";
  if (offset == -1) offset = parse->offset;

  do
  {
    if (parse->in_file)
    {
      /* Open the file and find which line we're on */

      fp = fopen(parse->context, "r");
      if (fp == NULL) {
	ne_warn("Unable to open context %s", parse->context);
	break;
      }
      while (fgets(line, sizeof(line), fp) != NULL)
      {
	count += strlen(line);
	if (strchr(line, '\n') != NULL)
	  lineno++;
	if (count > offset) break;
      }
      fclose (fp);
      snprintf (buf, blen, "[%s:%d]", parse->context, lineno);
    }
    else
    {
      snprintf (buf, blen, "[%s:%d]", parse->context, offset);
    }
    err = 0;
  } while (0);
  if (err)
    snprintf (buf, blen, "[-E- %s:%d]", parse->context, offset);

  return buf;
}

NEOERR *cs_parse_string (CSPARSE *parse, char *ibuf, size_t ibuf_len)
{
  NEOERR *err = STATUS_OK;
  STACK_ENTRY *entry;
  char *p;
  char *token;
  int done = 0;
  int i, n;
  char *arg;
  int initial_stack_depth;
  int initial_offset;
  char tmp[256];

  initial_stack_depth = uListLength(parse->stack);
  initial_offset = parse->offset;

  parse->offset = 0;
  while (!done)
  {
    /* Stage 1: Find <?cs starter */
    i = find_open_delim (ibuf, parse->offset, ibuf_len);
    if (i >= 0)
    {
      ibuf[i] = '\0';
      /* Create literal with data up until start delim */
      /* ne_warn ("literal -> %d-%d", parse->offset, i);  */
      err = (*(Commands[0].parse_handler))(parse, 0, &(ibuf[parse->offset]));
      /* skip delim */
      token = &(ibuf[i+5]);
      while (*token && isspace(*token)) token++;

      p = strstr (token, "?>");
      if (p == NULL)
      {
	return nerr_raise (NERR_PARSE, "%s Missing end ?> at %s", 
	    find_context(parse, i, tmp, sizeof(tmp)), &(ibuf[parse->offset]));
      }
      *p = '\0';
      if (strstr (token, "<?") != NULL)
      {
	return nerr_raise (NERR_PARSE, "%s Missing end ?> at %s", 
	    find_context(parse, i, tmp, sizeof(tmp)), 
	    token);
      }
      parse->offset = p - ibuf + 2;
      if (token[0] != '#') /* handle comments */
      {
	for (i = 1; Commands[i].cmd; i++)
	{
	  n = Commands[i].cmdlen;
	  if (!strncasecmp(token, Commands[i].cmd, n))
	  {
	    if ((Commands[i].has_arg && ((token[n] == ':') || (token[n] == '!')))
		|| (token[n] == ' ' || token[n] == '\0'))
	    {
	      err = uListGet (parse->stack, -1, (void **)&entry);
	      if (err != STATUS_OK) goto cs_parse_done;
	      if (!(Commands[i].allowed_state & entry->state))
	      {
		return nerr_raise (NERR_PARSE, 
		    "%s Command %s not allowed in %d", Commands[i].cmd, 
		    find_context(parse, -1, tmp, sizeof(tmp)), 
		    entry->state);
	      }
	      if (Commands[i].has_arg)
	      {
		/* Need to parse out arg */
		arg = &token[n];
		err = (*(Commands[i].parse_handler))(parse, i, arg);
	      }
	      else
	      {
		err = (*(Commands[i].parse_handler))(parse, i, NULL);
	      }
	      if (err != STATUS_OK) goto cs_parse_done;
	      if (Commands[i].next_state & ST_POP)
	      {
		err = uListPop(parse->stack, (void **)&entry);
		if (err != STATUS_OK) goto cs_parse_done;
		if (entry->next_tree)
		  parse->current = entry->next_tree;
		else
		  parse->current = entry->tree;
		free(entry);
	      }
	      if ((Commands[i].next_state & ~ST_POP) != ST_SAME)
	      {
		entry = (STACK_ENTRY *) calloc (1, sizeof (STACK_ENTRY));
		if (entry == NULL)
		  return nerr_raise (NERR_NOMEM, 
		      "%s Unable to allocate memory for stack entry",
		      find_context(parse, -1, tmp, sizeof(tmp)));
		entry->state = Commands[i].next_state;
		entry->tree = parse->current;
		entry->location = parse->offset;
		err = uListAppend(parse->stack, entry);
		if (err != STATUS_OK) {
		  free (entry);
		  goto cs_parse_done;
		}
	      }
	      break;
	    }
	  }
	}
	if (Commands[i].cmd == NULL)
	{
	  return nerr_raise (NERR_PARSE, "%s Unknown command %s",
	      find_context(parse, -1, tmp, sizeof(tmp)), token);
	}
      }
    }
    else
    {
      /* Create literal with all remaining data */
      err = (*(Commands[0].parse_handler))(parse, 0, &(ibuf[parse->offset]));
      done = 1;
    }
  }
  /* Should we check the parse stack here? */
  while (uListLength(parse->stack) > initial_stack_depth)
  {
    err = uListPop(parse->stack, (void **)&entry);
    if (err != STATUS_OK) goto cs_parse_done;
    if (entry->state & (ST_IF | ST_ELSE))
      return nerr_raise (NERR_PARSE, "%s Non-terminted if clause",
	  find_context(parse, entry->location, tmp, sizeof(tmp)));
    if (entry->state & ST_EACH)
      return nerr_raise (NERR_PARSE, "%s Non-terminted each clause",
	  find_context(parse, entry->location, tmp, sizeof(tmp)));
  }

cs_parse_done:
  parse->offset = initial_offset;
  return nerr_pass(err);
}


static HDF *var_lookup_obj (CSPARSE *parse, char *name)
{
  CS_LOCAL_MAP *map;
  char *c;

  map = parse->locals;
  c = strchr (name, '.');
  if (c != NULL) *c = '\0';
  while (map != NULL)
  {
    if (!strcmp (map->name, name))
    {
      if (c == NULL)
      {
	return map->value.h;
      }
      else
      {
	*c = '.';
	return hdf_get_obj (map->value.h, c+1);
      }
    }
    map = map->next;
  }
  if (c != NULL) *c = '.';
  return hdf_get_obj (parse->hdf, name);
}

/* Ugh, I have to write the same walking code because I can't grab the
 * object for writing, as it might not exist... */
static NEOERR *var_set_value (CSPARSE *parse, char *name, char *value)
{
  CS_LOCAL_MAP *map;
  char *c;

  map = parse->locals;
  c = strchr (name, '.');
  if (c != NULL) *c = '\0';
  while (map != NULL)
  {
    if (!strcmp (map->name, name))
    {
      if (c == NULL)
      {
	return nerr_pass (hdf_set_value (map->value.h, NULL, value));
      }
      else
      {
	*c = '.';
	return nerr_pass (hdf_set_value (map->value.h, c+1, value));
      }
    }
    map = map->next;
  }
  if (c != NULL) *c = '.';
  return nerr_pass (hdf_set_value (parse->hdf, name, value));
}

static char *var_lookup (CSPARSE *parse, char *name)
{
  HDF *hdf;
  hdf = var_lookup_obj (parse, name);
  if (hdf != NULL)
    return hdf_obj_value (hdf);
  else
    return NULL;
}

static NEOERR *var_int_lookup (CSPARSE *parse, char *name, int *value)
{
  char *vs;

  vs = var_lookup (parse, name);

  if (vs == NULL)
    *value = 0;
  else
    *value = atoi(vs);

  return STATUS_OK;
}

static NEOERR *literal_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  CSTREE *node;

  /* ne_warn ("literal: %s", arg); */
  err = alloc_node (&node);
  if (err) return nerr_pass(err);
  node->cmd = cmd;
  node->arg1.type = CS_TYPE_STRING;
  node->arg1.s = arg;
  *(parse->next) = node;
  parse->next = &(node->next);
  parse->current = node;

  return STATUS_OK;
}

static NEOERR *literal_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  NEOERR *err = STATUS_OK;

  if (node->arg1.s != NULL)
    err = parse->output_cb (parse->output_ctx, node->arg1.s);
  *next = node->next;
  return nerr_pass(err);
}

static NEOERR *name_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  CSTREE *node;
  char *a, *s;
  char tmp[256];

  /* ne_warn ("name: %s", arg); */
  err = alloc_node (&node);
  if (err) return nerr_pass(err);
  node->cmd = cmd;
  if (arg[0] == '!')
    node->flags |= CSF_REQUIRED;
  arg++;
  /* Validate arg is a var (regex /^[#" ]$/) */
  a = neos_strip(arg);
  s = strpbrk(a, "#\" <>");
  if (s != NULL)
  {
    dealloc_node(&node);
    return nerr_raise (NERR_PARSE, "%s Invalid character in var name %s: %c", 
	find_context(parse, -1, tmp, sizeof(tmp)),
	a, s[0]);
  }

  node->arg1.type = CS_TYPE_VAR;
  node->arg1.s = a;
  *(parse->next) = node;
  parse->next = &(node->next);
  parse->current = node;
  
  return STATUS_OK;
}

static NEOERR *name_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  NEOERR *err = STATUS_OK;
  HDF *obj;
  char *v;

  if (node->arg1.type == CS_TYPE_VAR && node->arg1.s != NULL)
  {
    obj = var_lookup_obj (parse, node->arg1.s);
    if (obj != NULL)
    {
      v = hdf_obj_name(obj);
      err = parse->output_cb (parse->output_ctx, v);
    }
  }
  *next = node->next;
  return nerr_pass(err);
}

static NEOERR *var_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  CSTREE *node;
  char *a, *s;
  char tmp[256];

  /* ne_warn ("var: %s", arg); */
  err = alloc_node (&node);
  if (err) return nerr_pass(err);
  node->cmd = cmd;
  if (arg[0] == '!')
    node->flags |= CSF_REQUIRED;
  arg++;
  /* Validate arg is a var (regex /^[#" ]$/) */
  a = neos_strip(arg);
  s = strpbrk(a, "#\" <>");
  if (s != NULL)
  {
    dealloc_node(&node);
    return nerr_raise (NERR_PARSE, "%s Invalid character in var name %s: %c", 
	find_context(parse, -1, tmp, sizeof(tmp)),
	a, s[0]);
  }

  node->arg1.type = CS_TYPE_VAR;
  node->arg1.s = a;
  *(parse->next) = node;
  parse->next = &(node->next);
  parse->current = node;
  
  return STATUS_OK;
}

static NEOERR *var_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  NEOERR *err = STATUS_OK;
  char *v;

  if (node->arg1.type == CS_TYPE_VAR && node->arg1.s != NULL)
  {
    v = var_lookup (parse, node->arg1.s);
    if (v != NULL)
      err = parse->output_cb (parse->output_ctx, v);
  }
  *next = node->next;
  return nerr_pass(err);
}

static NEOERR *evar_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  CSTREE *node;
  char *a, *s;
  char *save_context;
  int save_infile;
  char tmp[256];

  /* ne_warn ("evar: %s", arg); */
  err = alloc_node (&node);
  if (err) return nerr_pass(err);
  node->cmd = cmd;
  if (arg[0] == '!')
    node->flags |= CSF_REQUIRED;
  arg++;
  /* Validate arg is a var (regex /^[#" ]$/) */
  a = neos_strip(arg);
  s = strpbrk(a, "#\" <>");
  if (s != NULL)
  {
    dealloc_node(&node);
    return nerr_raise (NERR_PARSE, "%s Invalid character in var name %s: %c", 
	find_context(parse, -1, tmp, sizeof(tmp)),
	a, s[0]);
  }

  s = hdf_get_value (parse->hdf, a, NULL);
  if (node->flags & CSF_REQUIRED && s == NULL) 
  {
    dealloc_node(&node);
    return nerr_raise (NERR_NOT_FOUND, "%s Unable to evar empty variable %s",
	find_context(parse, -1, tmp, sizeof(tmp)), a);
  }

  node->arg1.type = CS_TYPE_VAR;
  node->arg1.s = a;
  *(parse->next) = node;
  parse->next = &(node->next);
  parse->current = node;

  save_context = parse->context;
  save_infile = parse->in_file;
  parse->context = a;
  parse->in_file = 0;
  err = cs_parse_string (parse, s, strlen(s));
  parse->context = save_context;
  parse->in_file = save_infile;

  return nerr_pass (err);
}


static NEOERR *parse_arg (CSPARSE *parse, char *buf, CSARG *arg, char **remain)
{
  char *p;
  char tmp[256];

  while (*buf && isspace(*buf)) buf++;
  if (buf[0] == '#')
  {
    arg->type = CS_TYPE_NUM;
    arg->n = strtol(buf+1, remain, 0);
    if (*remain == buf+1)
    {
      arg->type = CS_TYPE_VAR_NUM;
      arg->s = buf+1;
      *remain = strpbrk(buf+1, "?<>=!#-+|&,) ");
      if (*remain == buf+1)
	return nerr_raise (NERR_PARSE, "%s Missing arg after #: %s",
	    find_context(parse, -1, tmp, sizeof(tmp)), buf);

    }
  }
  else if (buf[0] == '"')
  {
    arg->type = CS_TYPE_STRING;
    arg->s = buf + 1;
    p = strchr (buf+1, '"');
    if (p == NULL)
      return nerr_raise (NERR_PARSE, "%s Missing end of string: %s", 
	  find_context(parse, -1, tmp, sizeof(tmp)), buf);
    *p = '\0';
    *remain = p+1;
  }
  else
  {
    arg->type = CS_TYPE_VAR;
    arg->s = buf;
    *remain = strpbrk(buf, "?<>=!#-+|&,) ");
    if (*remain == buf)
      return nerr_raise (NERR_PARSE, "%s Var arg specified with no varname: %s",
	  find_context(parse, -1, tmp, sizeof(tmp)), buf);
  }
  return STATUS_OK;
}

static NEOERR *parse_expr (CSPARSE *parse, char *arg, CSTREE *node)
{
  NEOERR *err;
  int x;
  char *r;
  char tmp[256];

  arg = neos_strip(arg);
  if (arg[0] == '\0')
    return nerr_raise (NERR_PARSE, "%s No expression", 
	  find_context(parse, -1, tmp, sizeof(tmp)));
  if (arg[0] == '!')
  {
    node->op = CS_OP_NOT;
    arg++;
  }

  err = parse_arg(parse, arg, &(node->arg1), &r);
  if (err != STATUS_OK) return nerr_pass(err);

  /* Unary operators */
  if (r == NULL || r[0] == '\0')
  {
    if (node->op != CS_OP_NOT)
      node->op = CS_OP_EXISTS;
  }
  else
  {
    if (node->op == CS_OP_NOT)
      return nerr_raise (NERR_PARSE, "%s No expression", 
	  find_context(parse, -1, tmp, sizeof(tmp)));
    x = 0;
    while (r[x] && isspace(r[x])) x++;
    if (r[x] == '=' && r[x+1] == '=')
    {
      node->op = CS_OP_EQUAL;
      x+=2;
    }
    if (r[x] == '!' && r[x+1] == '=')
    {
      node->op = CS_OP_NEQUAL;
      x+=2;
    }
    else if (r[x] == '<' && r[x+1] == '=')
    {
      node->op = CS_OP_LTE;
      x+=2;
    }
    else if (r[x] == '>' && r[x+1] == '=')
    {
      node->op = CS_OP_GTE;
      x+=2;
    }
    else if (r[x] == '&' && r[x+1] == '&')
    {
      node->op = CS_OP_AND;
      x+=2;
    }
    else if (r[x] == '|' && r[x+1] == '|')
    {
      node->op = CS_OP_OR;
      x+=2;
    }
    else if (r[x] == '<')
    {
      node->op = CS_OP_LT;
      x++;
    }
    else if (r[x] == '>')
    {
      node->op = CS_OP_LT;
      x++;
    }
    else if (r[x] == '+')
    {
      node->op = CS_OP_ADD;
      x++;
    }
    else if (r[x] == '-')
    {
      node->op = CS_OP_SUB;
      x++;
    }
    else if (r[x] == '*')
    {
      node->op = CS_OP_MULT;
      x++;
    }
    else if (r[x] == '/')
    {
      node->op = CS_OP_DIV;
      x++;
    }
    else if (r[x] == '%')
    {
      node->op = CS_OP_MOD;
      x++;
    }
    /* HACK: There might not have been room to NULL terminate the var
     * arg1, so we do it here after we've parsed the op */
    r[0] = '\0';
    r += x;
    while (*r && isspace(*r)) r++;
    err = parse_arg(parse, r, &(node->arg2), &r);
    if (err != STATUS_OK) return nerr_pass(err);
  }
  return STATUS_OK;
}

static NEOERR *if_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  CSTREE *node;

  /* ne_warn ("if: %s", arg); */
  err = alloc_node (&node);
  if (err != STATUS_OK) return nerr_pass(err);
  node->cmd = cmd;
  arg++;

  err = parse_expr (parse, arg, node);
  if (err != STATUS_OK)
  {
    dealloc_node(&node);
    return nerr_pass(err);
  }

  *(parse->next) = node;
  parse->next = &(node->case_0);
  parse->current = node;

  return STATUS_OK;
}

static NEOERR *arg_eval (CSPARSE *parse, CSARG *arg, char **value)
{
  switch (arg->type)
  {
    case CS_TYPE_STRING:
      *value = arg->s;
      break;
    case CS_TYPE_VAR:
      {
	/* err = hdf_get_value (parse->hdf, arg->s, value, 0); */
	*value = var_lookup (parse, arg->s);
      }
      break;
    case CS_TYPE_NUM:
    case CS_TYPE_VAR_NUM:
      /* These types should force numeric evaluation... so nothing
       * should get here */
    default:
      ne_warn ("Unsupported type %s in arg_eval_num", arg->type);
      *value = 0;
      break;
  }
  return STATUS_OK;
}

static NEOERR *arg_eval_num (CSPARSE *parse, CSARG *arg, long int *value)
{
  NEOERR *err;
  switch (arg->type)
  {
    case CS_TYPE_STRING:
      *value = strtol(arg->s, NULL, 0);
      break;
    case CS_TYPE_NUM:
      *value = arg->n;
      break;

    case CS_TYPE_VAR:
    case CS_TYPE_VAR_NUM:
      {
	int v;
	/* err = hdf_get_int_value (parse->hdf, arg->s, &v, 0); */
	err = var_int_lookup (parse, arg->s, &v);
	if (err != STATUS_OK) return nerr_pass (err);
	*value = v;
      }
      break;
    default:
      ne_warn ("Unsupported type %s in arg_eval_num", arg->type);
      *value = 0;
      break;
  }
  return STATUS_OK;
}

static NEOERR *if_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  NEOERR *err = STATUS_OK;
  int eval_true = 0;

  if ((node->op == CS_OP_EXISTS) || (node->op == CS_OP_NOT))
  {
    char *vs;
    int vn;
    if (node->arg1.type == CS_TYPE_STRING)
      eval_true = 1;
    else if (node->arg1.type == CS_TYPE_NUM)
      eval_true = (node->arg1.n) ? 1 : 0;
    else if (node->arg1.type == CS_TYPE_VAR)
    {
      /* err = hdf_get_value (parse->hdf, node->arg1.s, &vs, NULL); */
      vs = var_lookup (parse, node->arg1.s);
      if (!((vs == NULL) || (vs[0] == '\0')))
      {
	eval_true = 1;
      }
    }
    else if (node->arg1.type == CS_TYPE_VAR_NUM)
    {
      /* err = hdf_get_int_value (parse->hdf, node->arg1.s, &vn, 0); */
      err = var_int_lookup (parse, node->arg1.s, &vn);
      if (err != STATUS_OK) return nerr_pass(err);
      eval_true = (vn) ? 1 : 0;
    }
    if (node->op == CS_OP_NOT) eval_true = eval_true ? 0 : 1;
  }
  else
  {
    if ((node->arg1.type == CS_TYPE_NUM) || 
	(node->arg2.type == CS_TYPE_NUM) ||
	(node->arg1.type == CS_TYPE_VAR_NUM) || 
	(node->arg2.type == CS_TYPE_VAR_NUM) ||
	(node->op == CS_OP_AND) ||
	(node->op == CS_OP_OR) ||
	(node->op == CS_OP_ADD) ||
	(node->op == CS_OP_SUB) || 
	(node->op == CS_OP_MULT) ||
	(node->op == CS_OP_DIV) ||
	(node->op == CS_OP_MOD))
    {
      /* Numeric evaluation */
      long int n1, n2;
      err = arg_eval_num (parse, &(node->arg1), &n1);
      err = arg_eval_num (parse, &(node->arg2), &n2);
      switch (node->op)
      {
	case CS_OP_EQUAL:
	  eval_true = (n1 == n2) ? 1 : 0;
	  break;
	case CS_OP_NEQUAL:
	  eval_true = (n1 != n2) ? 1 : 0;
	  break;
	case CS_OP_LT:
	  eval_true = (n1 < n2) ? 1 : 0;
	  break;
	case CS_OP_LTE:
	  eval_true = (n1 <= n2) ? 1 : 0;
	  break;
	case CS_OP_GT:
	  eval_true = (n1 > n2) ? 1 : 0;
	  break;
	case CS_OP_GTE:
	  eval_true = (n1 >= n2) ? 1 : 0;
	  break;
	case CS_OP_AND:
	  eval_true = (n1 && n2) ? 1 : 0;
	  break;
	case CS_OP_OR:
	  eval_true = (n1 || n2) ? 1 : 0;
	  break;
	case CS_OP_ADD:
	  eval_true = (n1 + n2) ? 1 : 0;
	  break;
	case CS_OP_SUB:
	  eval_true = (n1 - n2) ? 1 : 0;
	  break;
	case CS_OP_MULT:
	  eval_true = (n1 * n2) ? 1 : 0;
	  break;
	case CS_OP_DIV:
	  if (n2 == 0) eval_true = 1;
	  else eval_true = (n1 / n2) ? 1 : 0;
	  break;
	case CS_OP_MOD:
	  if (n2 == 0) eval_true = 1;
	  else eval_true = (n1 % n2) ? 1 : 0;
	  break;
	default:
	  ne_warn ("Unsupported op %s in if_eval", node->op);
	  break;
      }
    }
    else
    {
      char *s1, *s2;
      int out;
      err = arg_eval (parse, &(node->arg1), &s1);
      err = arg_eval (parse, &(node->arg2), &s2);
      /* ne_warn("arg1 = %s, arg2 = %s", s1, s2); */
      if ((s1 == NULL) || (s2 == NULL))
      {
	switch (node->op)
	{
	  case CS_OP_EQUAL:
	    eval_true = (s1 == s2) ? 1 : 0;
	    break;
	  case CS_OP_NEQUAL:
	    eval_true = (s1 != s2) ? 1 : 0;
	    break;
	  case CS_OP_LT:
	    eval_true = ((s1 == NULL) && (s2 != NULL)) ? 1 : 0;
	    break;
	  case CS_OP_LTE:
	    eval_true = (s1 == NULL) ? 1 : 0;
	    break;
	  case CS_OP_GT:
	    eval_true = ((s1 != NULL) && (s2 == NULL)) ? 1 : 0;
	    break;
	  case CS_OP_GTE:
	    eval_true = (s2 == NULL) ? 1 : 0;
	    break;
	  default:
	    ne_warn ("Unsupported op %s in if_eval", node->op);
	    break;
	}
      }
      else
      {
	out = strcmp (s1, s2);
	switch (node->op)
	{
	  case CS_OP_EQUAL:
	    eval_true = (!out) ? 1 : 0;
	    break;
	  case CS_OP_NEQUAL:
	    eval_true = (out) ? 1 : 0;
	    break;
	  case CS_OP_LT:
	    eval_true = (out < 0) ? 1 : 0;
	    break;
	  case CS_OP_LTE:
	    eval_true = (out <= 0) ? 1 : 0;
	    break;
	  case CS_OP_GT:
	    eval_true = (out > 0) ? 1 : 0;
	    break;
	  case CS_OP_GTE:
	    eval_true = (out >= 0) ? 1 : 0;
	    break;
	  case CS_OP_AND:
	  case CS_OP_OR:
	  default:
	    ne_warn ("Unsupported op %s in if_eval", node->op);
	    break;
	}
      }
    }
  }

  if (eval_true)
  {
    err = render_node (parse, node->case_0);
  }
  else if (node->case_1 != NULL)
  {
    err = render_node (parse, node->case_1);
  }
  *next = node->next;
  return nerr_pass (err);
}

static NEOERR *else_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  STACK_ENTRY *entry;

  /* ne_warn ("else"); */
  err = uListGet (parse->stack, -1, (void **)&entry);
  if (err != STATUS_OK) return nerr_pass(err);

  parse->next = &(entry->tree->case_1);
  parse->current = entry->tree;
  return STATUS_OK;
}

static NEOERR *elif_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  STACK_ENTRY *entry;

  /* ne_warn ("elif: %s", arg); */
  err = uListGet (parse->stack, -1, (void **)&entry);
  if (err != STATUS_OK) return nerr_pass(err);

  if (entry->next_tree == NULL)
    entry->next_tree = entry->tree;

  parse->next = &(entry->tree->case_1);

  err = if_parse(parse, cmd, arg);
  entry->tree = parse->current;
  return nerr_pass(err);
}

static NEOERR *endif_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  STACK_ENTRY *entry;

  /* ne_warn ("endif"); */
  err = uListGet (parse->stack, -1, (void **)&entry);
  if (err != STATUS_OK) return nerr_pass(err);

  if (entry->next_tree)
    parse->next = &(entry->next_tree->next);
  else
    parse->next = &(entry->tree->next);
  parse->current = entry->tree;
  return STATUS_OK;
}

static NEOERR *each_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  CSTREE *node;
  char *lvar;
  char *p;
  char tmp[256];

  err = alloc_node (&node);
  if (err) return nerr_pass(err);
  node->cmd = cmd;
  if (arg[0] == '!')
    node->flags |= CSF_REQUIRED;
  arg++;

  p = lvar = neos_strip(arg);
  while (*p && !isspace(*p) && *p != '=') p++;
  if (*p == '\0')
  {
    dealloc_node(&node);
    return nerr_raise (NERR_PARSE, 
	"%s Improperly formatted each directive: %s", 
	find_context(parse, -1, tmp, sizeof(tmp)), arg);
  }
  if (*p != '=')
  {
    *p++ = '\0';
    while (*p && *p != '=') p++;
    if (*p == '\0')
    {
      dealloc_node(&node);
      return nerr_raise (NERR_PARSE, 
	  "%s Improperly formatted each directive: %s", 
	  find_context(parse, -1, tmp, sizeof(tmp)), arg);
    }
    p++;
  }
  else
  {
    *p++ = '\0';
  }
  while (*p && isspace(*p)) p++;
  if (*p == '\0')
  {
    dealloc_node(&node);
    return nerr_raise (NERR_PARSE, 
	"%s Improperly formatted each directive: %s", 
	find_context(parse, -1, tmp, sizeof(tmp)), arg);
  }
  node->arg1.type = CS_TYPE_VAR;
  node->arg1.s = lvar;

  node->arg2.type = CS_TYPE_VAR;
  node->arg2.s = p;

  /* ne_warn ("each %s %s", lvar, p); */

  *(parse->next) = node;
  parse->next = &(node->case_0);
  parse->current = node;

  return STATUS_OK;
}

static NEOERR *each_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  NEOERR *err = STATUS_OK;
  CS_LOCAL_MAP each_map;
  HDF *var, *child;

  var = var_lookup_obj (parse, node->arg2.s);

  if (var != NULL)
  {
    /* Init and install local map */
    each_map.is_map = 1;
    each_map.name = node->arg1.s;
    each_map.next = parse->locals;
    parse->locals = &each_map;

    do
    {
      child = hdf_obj_child (var);
      while (child != NULL)
      {
	each_map.value.h = child;
	err = render_node (parse, node->case_0);
	if (err != STATUS_OK) break;
	child = hdf_obj_next (child);
      }

    } while (0);

    /* Remove local map */
    parse->locals = each_map.next;
  }

  *next = node->next;
  return nerr_pass (err);
}

static NEOERR *endeach_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  STACK_ENTRY *entry;

  err = uListGet (parse->stack, -1, (void **)&entry);
  if (err != STATUS_OK) return nerr_pass(err);

  parse->next = &(entry->tree->next);
  parse->current = entry->tree;
  return STATUS_OK;
}

static NEOERR *include_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  CSTREE *node;
  char *a, *s;
  char tmp[256];

  err = alloc_node (&node);
  if (err) return nerr_pass(err);
  node->cmd = cmd;
  if (arg[0] == '!')
    node->flags |= CSF_REQUIRED;
  arg++;
  /* Validate arg is a var (regex /^[#" ]$/) */
  a = neos_strip(arg);
  /* ne_warn ("include: %s", a); */
  s = strpbrk(a, "# <>");
  if (s != NULL)
  {
    dealloc_node(&node);
    return nerr_raise (NERR_PARSE, 
	"%s Invalid character in include argument %s: %c", 
	find_context(parse, -1, tmp, sizeof(tmp)), a, s[0]);
  }

  /* Literal string or var */
  if (a[0] == '\"')
  {
    int l;
    a++;
    l = strlen(a);

    if (a[l - 1] == '\"')
    {
      a[l - 1] = '\0';
    }
    node->arg1.type = CS_TYPE_STRING;
    node->arg1.s = a;
    s = a;
  }
  else
  {
    s = hdf_get_value (parse->hdf, a, NULL);
    if (s == NULL) 
    {
      dealloc_node(&node);
      return nerr_raise (NERR_NOT_FOUND, 
	  "%s Unable to include empty variable %s", 
	  find_context(parse, -1, tmp, sizeof(tmp)), a);
    }
    node->arg1.type = CS_TYPE_VAR;
    node->arg1.s = a;
  }

  *(parse->next) = node;
  parse->next = &(node->next);
  parse->current = node;

  return nerr_pass (cs_parse_file (parse, s));
}

static NEOERR *def_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  CSTREE *node;
  CS_MACRO *macro;
  CSARG *carg, *larg = NULL;
  char *a = NULL, *s;
  char tmp[256];
  char name[256];
  int x = 0;

  err = alloc_node (&node);
  if (err) return nerr_pass(err);
  node->cmd = cmd;
  arg++;
  s = arg;
  while (*s && *s != ' ' && *s != '#' && *s != '(')
  {
    name[x++] = *s;
    s++;
  }
  name[x] = '\0';
  while (*s && isspace(*s)) s++;
  if (*s == '\0' || *s != '(')
  {
    dealloc_node(&node);
    return nerr_raise (NERR_PARSE, 
	"%s Missing left paren in macro def %s", 
	find_context(parse, -1, tmp, sizeof(tmp)), arg);
  }
  s++;
  /* Check to see if this is a redefinition */
  macro = parse->macros;
  while (macro != NULL)
  {
    if (!strcmp(macro->name, name))
    {
      dealloc_node(&node);
      return nerr_raise (NERR_PARSE, 
	  "%s Duplicate macro def for %s", 
	  find_context(parse, -1, tmp, sizeof(tmp)), arg);
    }
    macro = macro->next;
  }

  macro = (CS_MACRO *) calloc (1, sizeof (CS_MACRO));
  if (macro) macro->name = strdup(name);
  if (macro == NULL || macro->name == NULL)
  {
    dealloc_node(&node);
    dealloc_macro(&macro);
    return nerr_raise (NERR_NOMEM, 
	"%s Unable to allocate memory for CS_MACRO in def %s",
	find_context(parse, -1, tmp, sizeof(tmp)), arg);
  }

  while (*s)
  {
    carg = (CSARG *) calloc (1, sizeof(CSARG));
    if (carg == NULL)
    {
      err = nerr_raise (NERR_NOMEM, 
	  "%s Unable to allocate memory for CSARG in def %s",
	  find_context(parse, -1, tmp, sizeof(tmp)), arg);
      break;
    }
    if (larg == NULL)
    {
      macro->args = carg;
      larg = carg;
    }
    else
    {
      larg->next = carg;
      larg = carg;
    }
    macro->n_args++;
    err = parse_arg (parse, s, carg, &s);
    if (err) break;
    a = s;
    while (*s && isspace(*s)) s++;
    if (*s == ')') break;
    if (*s == ',') s++;
    *a = '\0';
    while (*s && isspace(*s)) s++;
  }
  if (!err && *s != ')')
  {
    err = nerr_raise (NERR_PARSE, 
	"%s Missing right paren in def %s",
	find_context(parse, -1, tmp, sizeof(tmp)), arg);
  }
  if (a) *a = '\0';
  if (err)
  {
    dealloc_node(&node);
    dealloc_macro(&macro);
    return nerr_pass(err);
  }

  macro->tree = node;
  if (parse->macros)
  {
    macro->next = parse->macros;
  }
  parse->macros = macro;

  *(parse->next) = node;
  parse->next = &(node->case_0);
  parse->current = node;

  return STATUS_OK;
}

static NEOERR *enddef_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  STACK_ENTRY *entry;

  err = uListGet (parse->stack, -1, (void **)&entry);
  if (err != STATUS_OK) return nerr_pass(err);

  parse->next = &(entry->tree->next);
  parse->current = entry->tree;
  return STATUS_OK;
}

static NEOERR *call_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  CSTREE *node;
  CS_MACRO *macro;
  CSARG *carg, *larg = NULL;
  char *s, *a = NULL;
  char tmp[256];
  char name[256];
  int x = 0;

  err = alloc_node (&node);
  if (err) return nerr_pass(err);
  node->cmd = cmd;
  arg++;
  s = arg;
  while (x < 256 && *s && *s != ' ' && *s != '#' && *s != '(')
  {
    name[x++] = *s;
    s++;
  }
  name[x] = '\0';
  while (*s && isspace(*s)) s++;
  if (*s == '\0' || *s != '(')
  {
    dealloc_node(&node);
    return nerr_raise (NERR_PARSE, 
	"%s Missing left paren in call %s", 
	find_context(parse, -1, tmp, sizeof(tmp)), arg);
  }
  s++;
  /* Check to see if this macro exists */
  macro = parse->macros;
  while (macro != NULL)
  {
    if (!strcmp(macro->name, name)) break;
    macro = macro->next;
  }
  if (macro == NULL)
  {
    dealloc_node(&node);
    return nerr_raise (NERR_PARSE, 
	"%s Undefined macro called: %s", 
	find_context(parse, -1, tmp, sizeof(tmp)), arg);
  }
  node->arg1.type = CS_TYPE_MACRO;
  node->arg1.macro = macro;

  x = 0;
  while (*s)
  {
    carg = (CSARG *) calloc (1, sizeof(CSARG));
    if (carg == NULL)
    {
      err = nerr_raise (NERR_NOMEM, 
	  "%s Unable to allocate memory for CSARG in call %s",
	  find_context(parse, -1, tmp, sizeof(tmp)), arg);
      break;
    }
    if (larg == NULL)
    {
      node->vargs = carg;
      larg = carg;
    }
    else
    {
      larg->next = carg;
      larg = carg;
    }
    x++;
    err = parse_arg (parse, s, carg, &s);
    if (err) break;
    a = s;
    while (*s && isspace(*s)) s++;
    if (*s == ')') break;
    if (*s == ',') s++;
    *a = '\0';
    while (*s && isspace(*s)) s++;
  }
  if (!err && *s != ')')
  {
    err = nerr_raise (NERR_PARSE, 
	"%s Missing right paren in def %s",
	find_context(parse, -1, tmp, sizeof(tmp)), arg);
  }
  if (!err && x != macro->n_args)
  {
    err = nerr_raise (NERR_PARSE, 
	"%s Incorrect number of arguments, expected %d, got %d in call to macro %s: %s",
	find_context(parse, -1, tmp, sizeof(tmp)), macro->n_args, x, 
	macro->name, arg);
  }
  if (err)
  {
    dealloc_node(&node);
    return nerr_pass(err);
  }
  if (a) *a = '\0';

  *(parse->next) = node;
  parse->next = &(node->next);
  parse->current = node;

  return STATUS_OK;
}

static NEOERR *call_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  NEOERR *err = STATUS_OK;
  CS_LOCAL_MAP *call_map, *map;
  CS_MACRO *macro;
  CSARG *carg, *darg;
  HDF *var;
  int x;

  macro = node->arg1.macro;
  call_map = (CS_LOCAL_MAP *) calloc (macro->n_args, sizeof(CS_LOCAL_MAP));
  if (call_map == NULL)
    return nerr_raise (NERR_NOMEM, 
	"Unable to allocate memory for call_map in call_eval of %s", 
	macro->name);

  darg = macro->args;
  carg = node->vargs;

  for (x = 0; x < macro->n_args; x++)
  {
    map = &call_map[x];
    if (x) call_map[x-1].next = map;

    map->name = darg->s;
    if (carg->type == CS_TYPE_STRING)
    {
      map->value.s = carg->s;
    }
    else if (carg->type == CS_TYPE_NUM)
    {
      map->value.n = carg->n;
    }
    else 
    {
      var = var_lookup_obj (parse, carg->s);
      map->value.h = var;
      map->is_map = 1;
    }
    map->next = parse->locals;

    darg = darg->next;
    carg = carg->next;
  }

  map = parse->locals;
  if (macro->n_args) parse->locals = call_map;
  err = render_node (parse, macro->tree->case_0);
  parse->locals = map;
  free (call_map);

  *next = node->next;
  return nerr_pass(err);
}

static NEOERR *set_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  CSTREE *node;
  CS_OP op = CS_OP_EXISTS;
  CSARG *carg, *larg = NULL;
  char *s, *a = NULL;
  char tmp[256];
  char name[256];
  int x = 0;

  err = alloc_node (&node);
  if (err) return nerr_pass(err);
  node->cmd = cmd;
  arg++;
  s = arg;
  while (x < 256 && *s && *s != ' ' && *s != '#' && *s != '=')
  {
    name[x++] = *s;
    s++;
  }
  name[x] = '\0';
  while (*s && isspace(*s)) s++;
  if (*s == '\0' || *s != '=')
  {
    dealloc_node(&node);
    return nerr_raise (NERR_PARSE, 
	"%s Missing equals in set %s", 
	find_context(parse, -1, tmp, sizeof(tmp)), arg);
  }
  s++;
  node->arg1.type = CS_TYPE_VAR;
  node->arg1.s = strdup(name);

  while (*s)
  {
    carg = (CSARG *) calloc (1, sizeof(CSARG));
    if (carg == NULL)
    {
      err = nerr_raise (NERR_NOMEM, 
	  "%s Unable to allocate memory for CSARG in set %s",
	  find_context(parse, -1, tmp, sizeof(tmp)), arg);
      break;
    }
    carg->op = op;
    op = CS_OP_EXISTS;
    if (larg == NULL)
    {
      node->vargs = carg;
      larg = carg;
    }
    else
    {
      larg->next = carg;
      larg = carg;
    }
    err = parse_arg (parse, s, carg, &s);
    a = s;
    if (err) break;
    /* Unary operators */
    if (s == NULL || s[0] == '\0')
    {
      if (op != CS_OP_NOT)
	op = CS_OP_EXISTS;
    }
    else
    {
      if (op == CS_OP_NOT)
	return nerr_raise (NERR_PARSE, "%s No expression", 
	    find_context(parse, -1, tmp, sizeof(tmp)));
      x = 0;
      while (s[x] && isspace(s[x])) x++;
      if (s[x] == '=' && s[x+1] == '=')
      {
	op = CS_OP_EQUAL;
	x+=2;
      }
      if (s[x] == '!' && s[x+1] == '=')
      {
	op = CS_OP_NEQUAL;
	x+=2;
      }
      else if (s[x] == '<' && s[x+1] == '=')
      {
	op = CS_OP_LTE;
	x+=2;
      }
      else if (s[x] == '>' && s[x+1] == '=')
      {
	op = CS_OP_GTE;
	x+=2;
      }
      else if (s[x] == '&' && s[x+1] == '&')
      {
	op = CS_OP_AND;
	x+=2;
      }
      else if (s[x] == '|' && s[x+1] == '|')
      {
	op = CS_OP_OR;
	x+=2;
      }
      else if (s[x] == '<')
      {
	op = CS_OP_LT;
	x++;
      }
      else if (s[x] == '>')
      {
	op = CS_OP_LT;
	x++;
      }
      else if (s[x] == '+')
      {
	op = CS_OP_ADD;
	x++;
      }
      else if (s[x] == '-')
      {
	op = CS_OP_SUB;
	x++;
      }
      else if (s[x] == '*')
      {
	op = CS_OP_MULT;
	x++;
      }
      else if (s[x] == '/')
      {
	op = CS_OP_DIV;
	x++;
      }
      else if (s[x] == '%')
      {
	op = CS_OP_MOD;
	x++;
      }
      /* HACK: There might not have been room to NULL terminate the var
       * arg1, so we do it here after we've parsed the op */
      *a = '\0';
      s += x;
    }
    while (*s && isspace(*s)) s++;
  }
  if (err)
  {
    dealloc_node(&node);
    return nerr_pass(err);
  }
  if (a) *a = '\0';

  *(parse->next) = node;
  parse->next = &(node->next);
  parse->current = node;

  return STATUS_OK;
}

static NEOERR *set_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  NEOERR *err = STATUS_OK;
  CSARG *arg;
  STRING s_val;
  long int n_val = 0;
  BOOL is_num = FALSE;

  string_init (&s_val);
  arg = node->vargs;
  while (arg)
  {
    /* This should only happen on the first arg */
    if ((arg->op == CS_OP_EXISTS) || (arg->op == CS_OP_NOT))
    {
      char *vs;
      int vn;
      if (arg->type == CS_TYPE_STRING)
      {
	err = string_set (&s_val, arg->s);
      }
      else if (arg->type == CS_TYPE_NUM)
      {
	is_num = TRUE;
	n_val = arg->n;
      }
      else if (arg->type == CS_TYPE_VAR)
      {
	vs = var_lookup (parse, arg->s);
	if (vs == NULL)
	{
	  err = string_set (&s_val, "");
	}
	else
	{
	  err = string_set (&s_val, vs);
	}
      }
      else if (arg->type == CS_TYPE_VAR_NUM)
      {
	is_num = TRUE;
	err = var_int_lookup (parse, arg->s, &vn);
	if (err != STATUS_OK) return nerr_pass(err);
	n_val = vn;
      }
    }
    else
    {
      if (is_num ||
	  (arg->type == CS_TYPE_NUM) || 
	  (arg->type == CS_TYPE_VAR_NUM) || 
	  (arg->op == CS_OP_AND) ||
	  (arg->op == CS_OP_OR) ||
	  (arg->op == CS_OP_SUB) || 
	  (arg->op == CS_OP_MULT) ||
	  (arg->op == CS_OP_DIV) ||
	  (arg->op == CS_OP_MOD))
      {
	/* Numeric evaluation */
	long int n;

	if (is_num == FALSE)
	{
	  /* we have to convert the existing s_val to a number */
	  if (s_val.buf == NULL)
	    n_val = 0;
	  else
	    n_val = strtol(s_val.buf, NULL, 0);
	  is_num = TRUE;
	}

	err = arg_eval_num (parse, arg, &n);
	switch (arg->op)
	{
	  case CS_OP_EQUAL:
	    n_val = (n_val == n) ? 1 : 0;
	    break;
	  case CS_OP_NEQUAL:
	    n_val = (n_val != n) ? 1 : 0;
	    break;
	  case CS_OP_LT:
	    n_val = (n_val < n) ? 1 : 0;
	    break;
	  case CS_OP_LTE:
	    n_val = (n_val <= n) ? 1 : 0;
	    break;
	  case CS_OP_GT:
	    n_val = (n_val > n) ? 1 : 0;
	    break;
	  case CS_OP_GTE:
	    n_val = (n_val >= n) ? 1 : 0;
	    break;
	  case CS_OP_AND:
	    n_val = (n_val && n) ? 1 : 0;
	    break;
	  case CS_OP_OR:
	    n_val = (n_val || n) ? 1 : 0;
	    break;
	  case CS_OP_ADD:
	    n_val = (n_val + n);
	    break;
	  case CS_OP_SUB:
	    n_val = (n_val - n);
	    break;
	  case CS_OP_MULT:
	    n_val = (n_val * n);
	    break;
	  case CS_OP_DIV:
	    if (n == 0) n_val = UINT_MAX;
	    else n_val = (n_val / n);
	    break;
	  case CS_OP_MOD:
	    if (n == 0) n_val = UINT_MAX;
	    else n_val = (n_val % n);
	    break;
	  default:
	    ne_warn ("Unsupported op %s in set_eval", arg->op);
	    break;
	}
      }
      else
      {
	char *s, *vs;
	int out;
	err = arg_eval (parse, arg, &s);
	/* ne_warn("arg1 = %s, arg2 = %s", s_val, s); */
	if ((s_val.buf == NULL) || (s == NULL))
	{
	  switch (arg->op)
	  {
	    case CS_OP_EQUAL:
	      vs = (s_val.buf == s) ? "1" : "0";
	      err = string_set (&s_val, vs);
	      break;
	    case CS_OP_NEQUAL:
	      vs = (s_val.buf != s) ? "1" : "0";
	      err = string_set (&s_val, vs);
	      break;
	    case CS_OP_LT:
	      vs = ((s_val.buf == NULL) && (s != NULL)) ? "1" : "0";
	      err = string_set (&s_val, vs);
	      break;
	    case CS_OP_LTE:
	      vs = (s_val.buf == NULL) ? "1" : "0";
	      err = string_set (&s_val, vs);
	      break;
	    case CS_OP_GT:
	      vs = ((s_val.buf != NULL) && (s == NULL)) ? "1" : "0";
	      err = string_set (&s_val, vs);
	      break;
	    case CS_OP_GTE:
	      vs = (s == NULL) ? "1" : "0";
	      err = string_set (&s_val, vs);
	      break;
	    case CS_OP_ADD:
	      if (s_val.buf == NULL) 
		err = string_set (&s_val, s);
	      break;
	    default:
	      ne_warn ("Unsupported op %s in set_eval", arg->op);
	      break;
	  }
	}
	else
	{
	  out = strcmp (s_val.buf, s);
	  switch (arg->op)
	  {
	    case CS_OP_EQUAL:
	      vs = (!out) ? "1" : "0";
	      err = string_set (&s_val, vs);
	      break;
	    case CS_OP_NEQUAL:
	      vs = (out) ? "1" : "0";
	      err = string_set (&s_val, vs);
	      break;
	    case CS_OP_LT:
	      vs = (out < 0) ? "1" : "0";
	      err = string_set (&s_val, vs);
	      break;
	    case CS_OP_LTE:
	      vs = (out <= 0) ? "1" : "0";
	      err = string_set (&s_val, vs);
	      break;
	    case CS_OP_GT:
	      vs = (out > 0) ? "1" : "0";
	      err = string_set (&s_val, vs);
	      break;
	    case CS_OP_GTE:
	      vs = (out >= 0) ? "1" : "0";
	      err = string_set (&s_val, vs);
	      break;
	    case CS_OP_ADD:
	      err = string_append (&s_val, s);
	      break;
	    default:
	      ne_warn ("Unsupported op %s in set_eval", arg->op);
	      break;
	  }
	}
      }
    }
    if (err != STATUS_OK) return nerr_pass(err);
    arg = arg->next;
  }

  if (is_num)
  { 
    char buf[256];
    snprintf (buf, sizeof(buf), "%ld", n_val);
    err = var_set_value (parse, node->arg1.s, buf);
  }
  else
  {
    if (s_val.buf)
    {
      err = var_set_value (parse, node->arg1.s, s_val.buf);
    }
  }
  if (s_val.buf) string_clear (&s_val);

  *next = node->next;
  return nerr_pass (err);
}

static NEOERR *skip_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  *next = node->next;
  return STATUS_OK;
}

static NEOERR *render_node (CSPARSE *parse, CSTREE *node)
{
  NEOERR *err;

  while (node != NULL)
  {
    /* ne_warn ("%s %08x", Commands[node->cmd].cmd, node); */
    err = (*(Commands[node->cmd].eval_handler))(parse, node, &node);
  }
  return STATUS_OK;
}

NEOERR *cs_render (CSPARSE *parse, void *ctx, CSOUTFUNC cb)
{
  CSTREE *node;

  if (parse->tree == NULL)
    return nerr_raise (NERR_ASSERT, "No parse tree exists");

  parse->output_ctx = ctx;
  parse->output_cb = cb;

  node = parse->tree;
  return nerr_pass (render_node(parse, node));
}

static NEOERR *dump_node (CSPARSE *parse, CSTREE *node, int depth)
{
  while (node != NULL)
  {
    printf ("%*s %s ", depth, "", Commands[node->cmd].cmd);
    if (node->cmd)
    {
      if (node->arg1.type)
      {
	if (node->arg1.type == CS_TYPE_NUM)
	{
	  printf ("%ld ", node->arg1.n);
	}
	else if (node->arg1.type == CS_TYPE_MACRO)
	{
	  printf ("%s ", node->arg1.macro->name);
	}
	else 
	{
	  printf ("%s ", node->arg1.s);
	}
      }
      if (node->arg2.type)
      {
	if (node->arg2.type == CS_TYPE_NUM)
	{
	  printf ("%ld", node->arg2.n);
	}
	else 
	{
	  printf ("%s", node->arg2.s);
	}
      }
      if (node->vargs)
      {
	CSARG *arg;
	arg = node->vargs;
	while (arg)
	{
	  if (arg->type == CS_TYPE_NUM)
	  {
	    printf ("%ld ", arg->n);
	  }
	  else 
	  {
	    printf ("%s ", arg->s);
	  }
	  arg = arg->next;
	}
      }
    }
    printf("\n");
    if (node->case_0)
    {
      printf ("%*s %s\n", depth, "", "Case 0");
      dump_node (parse, node->case_0, depth+1);
    }
    if (node->case_1)
    {
      printf ("%*s %s\n", depth, "", "Case 1");
      dump_node (parse, node->case_1, depth+1);
    }
    node = node->next;
  }
  return STATUS_OK;
}

NEOERR *cs_dump (CSPARSE *parse)
{
  CSTREE *node;

  if (parse->tree == NULL)
    return nerr_raise (NERR_ASSERT, "No parse tree exists");

  node = parse->tree;
  return nerr_pass (dump_node (parse, node, 0));
}

static char *node_name (CSTREE *node)
{
  static char buf[256];

  if (node == NULL)
    snprintf (buf, sizeof(buf), "NULL");
  else
    snprintf (buf, sizeof(buf), "%s_%08x", Commands[node->cmd].cmd, 
	node->node_num);

  return buf;
}

static char *repr_string_alloc (char *s)
{
  int l,x,i;
  int nl = 0;
  char *rs;

  if (s == NULL)
  {
    return strdup("NULL");
  }

  l = strlen(s);
  for (x = 0; x < l; x++)
  {
    if (isprint(s[x]) && s[x] != '"' && s[x] != '\\')
    {
      nl++;
    }
    else
    {
      if (s[x] == '\n' || s[x] == '\t' || s[x] == '\r' || s[x] == '"' ||
	  s[x] == '\\')
      {
	nl += 2;
      }
      else nl += 4;
    }
  }

  rs = (char *) malloc ((nl+3) * sizeof(char));
  if (rs == NULL)
    return NULL;

  i = 0;
  rs[i++] = '"';
  for (x = 0; x < l; x++)
  {
    if (isprint(s[x]) && s[x] != '"' && s[x] != '\\')
    {
      rs[i++] = s[x];
    }
    else
    {
      rs[i++] = '\\';
      switch (s[x])
      {
	case '\n':
	  rs[i++] = 'n';
	  break;
	case '\t':
	  rs[i++] = 't';
	  break;
	case '\r':
	  rs[i++] = 'r';
	  break;
	case '"':
	  rs[i++] = '"';
	  break;
	case '\\':
	  rs[i++] = '\\';
	  break;
	default:
	  sprintf(&(rs[i]), "%03o", (s[x] & 0377));
	  i += 3;
	  break;
      }
    }
  }
  rs[i++] = '"';
  rs[i] = '\0';
  return rs;
}

static NEOERR *dump_node_pre_c (CSPARSE *parse, CSTREE *node, FILE *fp)
{
  NEOERR *err;

  while (node != NULL)
  {
    fprintf (fp, "CSTREE %s;\n", node_name(node));
    if (node->case_0)
    {
      err = dump_node_pre_c (parse, node->case_0, fp);
      if (err != STATUS_OK) nerr_pass (err);
    }
    if (node->case_1)
    {
      err = dump_node_pre_c (parse, node->case_1, fp);
      if (err != STATUS_OK) nerr_pass (err);
    }
    node = node->next;
  }
  return STATUS_OK;
}

static NEOERR *dump_node_c (CSPARSE *parse, CSTREE *node, FILE *fp)
{
  NEOERR *err;
  char *s;

  while (node != NULL)
  {
    fprintf (fp, "CSTREE %s =\n\t{%d, %d, %d, ", node_name(node), node->node_num, 
	node->cmd, node->flags);
    s = repr_string_alloc (node->arg1.s);
    if (s == NULL)
      return nerr_raise(NERR_NOMEM, "Unable to allocate space for repr");
    fprintf (fp, "\n\t  { %d, %s, %ld }, ", node->arg1.type, s, node->arg1.n);
    free(s);
    s = repr_string_alloc (node->arg2.s);
    if (s == NULL)
      return nerr_raise(NERR_NOMEM, "Unable to allocate space for repr");
    fprintf (fp, "\n\t  { %d, %s, %ld }, ", node->arg2.type, s, node->arg2.n);
    free(s);
    if (node->case_0)
      fprintf (fp, "\n\t%d, &%s, ", node->op, node_name(node->case_0));
    else
      fprintf (fp, "\n\t%d, NULL, ", node->op);
    if (node->case_1)
      fprintf (fp, "&%s, ", node_name(node->case_1));
    else
      fprintf (fp, "NULL, ");
    if (node->next)
      fprintf (fp, "&%s};\n\n", node_name(node->next));
    else
      fprintf (fp, "NULL};\n\n");
    if (node->case_0)
    {
      err = dump_node_c (parse, node->case_0, fp);
      if (err != STATUS_OK) nerr_pass (err);
    }
    if (node->case_1)
    {
      err = dump_node_c (parse, node->case_1, fp);
      if (err != STATUS_OK) nerr_pass (err);
    }
    node = node->next;
  }
  return STATUS_OK;
}

NEOERR *cs_dump_c (CSPARSE *parse, char *path)
{
  CSTREE *node;
  FILE *fp;
  NEOERR *err;

  if (parse->tree == NULL)
    return nerr_raise (NERR_ASSERT, "No parse tree exists");

  fp = fopen(path, "w");
  if (fp == NULL)
  {
    return nerr_raise (NERR_SYSTEM, 
	"Unable to open file %s for writing: [%d] %s", path, errno, 
	strerror(errno));
  }

  fprintf(fp, "/* Auto-generated file: DO NOT EDIT */\n");
  fprintf(fp, "#include <stdlib.h>\n\n");
  fprintf(fp, "#include \"cs.h\"\n");
  node = parse->tree;
  err = dump_node_pre_c (parse, node, fp);
  fprintf(fp, "\n");
  err = dump_node_c (parse, node, fp);
  fclose(fp);
  return nerr_pass (err);
}

