/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
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
  ST_LOOP =  1<<6,
} CS_STATE;

#define ST_ANYWHERE (ST_EACH | ST_ELSE | ST_IF | ST_GLOBAL | ST_DEF | ST_LOOP)

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
static NEOERR *loop_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *loop_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *endloop_parse (CSPARSE *parse, int cmd, char *arg);

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
  {"def",     sizeof("def")-1,     ST_ANYWHERE,     ST_DEF, 
    def_parse, skip_eval, 1},
  {"/def",    sizeof("/def")-1,    ST_DEF,          ST_POP,
    enddef_parse, skip_eval, 0},
  {"call",    sizeof("call")-1,    ST_ANYWHERE,     ST_SAME,
    call_parse, call_eval, 1},
  {"set",    sizeof("set")-1,    ST_ANYWHERE,     ST_SAME,
    set_parse, set_eval, 1},
  {"loop",    sizeof("loop")-1,    ST_ANYWHERE,     ST_LOOP,
    loop_parse, loop_eval, 1},
  {"/loop",    sizeof("/loop")-1,    ST_LOOP,     ST_POP,
    endloop_parse, skip_eval, 1},
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

static void dealloc_arg (CSARG **arg)
{
  CSARG *p;

  if (*arg == NULL) return;
  p = *arg;
  if (p->expr1) dealloc_arg (&(p->expr1));
  if (p->expr2) dealloc_arg (&(p->expr2));
  if (p->next) dealloc_arg (&(p->next));
  free(p);
  *arg = NULL;
}

static void dealloc_node (CSTREE **node)
{
  CSTREE *my_node;

  if (*node == NULL) return;
  my_node = *node;
  if (my_node->case_0) dealloc_node (&(my_node->case_0));
  if (my_node->case_1) dealloc_node (&(my_node->case_1));
  if (my_node->next) dealloc_node (&(my_node->next));
  if (my_node->vargs) dealloc_arg (&(my_node->vargs));
  if (my_node->arg1.expr1) dealloc_arg (&(my_node->arg1.expr1));
  if (my_node->arg1.expr2) dealloc_arg (&(my_node->arg1.expr2));
  if (my_node->arg1.next) dealloc_arg (&(my_node->arg1.next));
  if (my_node->arg2.expr1) dealloc_arg (&(my_node->arg2.expr1));
  if (my_node->arg2.expr2) dealloc_arg (&(my_node->arg2.expr2));
  if (my_node->arg2.next) dealloc_arg (&(my_node->arg2.next));

  free(my_node);
  *node = NULL;
}

static void dealloc_macro (CS_MACRO **macro)
{
  CS_MACRO *my_macro;

  if (*macro == NULL) return;
  my_macro = *macro;
  if (my_macro->name) free (my_macro->name);
  if (my_macro->args) dealloc_arg (&(my_macro->args));
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
  err = uListInit (&(my_parse->alloc), 10, 0);
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
  uListDestroy (&(my_parse->alloc), ULIST_FREE);

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
	p[4] && (p[4] == ' ' || p[4] == '\n' || p[4] == '\t' || p[4] == '\r'))
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
  char *ibuf;
  char *save_context;
  int save_infile;
  char fpath[_POSIX_PATH_MAX];

  if (path == NULL)
    return nerr_raise (NERR_ASSERT, "path is NULL");

  if (path[0] != '/')
  {
    err = hdf_search_path (parse->hdf, path, fpath);
    if (err != STATUS_OK) return nerr_pass(err);
    path = fpath;
  }

  err = ne_load_file (path, &ibuf);
  if (err) return nerr_pass (err);

  save_context = parse->context;
  parse->context = path;
  save_infile = parse->in_file;
  parse->in_file = 1;
  err = cs_parse_string(parse, ibuf, strlen(ibuf));
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

static char *expand_state (CS_STATE state)
{
  static char buf[256];

  if (state & ST_GLOBAL)
    return "GLOBAL";
  else if (state & ST_IF)
    return "IF";
  else if (state & ST_ELSE)
    return "ELSE";
  else if (state & ST_EACH)
    return "EACH";
  else if (state & ST_DEF)
    return "DEF";

  snprintf(buf, sizeof(buf), "Unknown state %d", state);
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

  err = uListAppend(parse->alloc, ibuf);
  if (err) 
  {
    free (ibuf);
    return nerr_pass (err);
  }

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
		|| (token[n] == ' ' || token[n] == '\0' || token[n] == '\r' || token[n] == '\n'))
	    {
	      err = uListGet (parse->stack, -1, (void **)&entry);
	      if (err != STATUS_OK) goto cs_parse_done;
	      if (!(Commands[i].allowed_state & entry->state))
	      {
		return nerr_raise (NERR_PARSE, 
		    "%s Command %s not allowed in %s", Commands[i].cmd, 
		    find_context(parse, -1, tmp, sizeof(tmp)), 
		    expand_state(entry->state));
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

static CS_LOCAL_MAP * lookup_map (CSPARSE *parse, char *name, char **rest)
{
  CS_LOCAL_MAP *map;
  char *c;

  /* This shouldn't happen, but it did once... */
  if (name == NULL) return NULL;
  map = parse->locals;
  c = strchr (name, '.');
  if (c != NULL) *c = '\0';
  *rest = c;
  while (map != NULL)
  {
    if (!strcmp (map->name, name))
    {
      if (c != NULL) *c = '.';
      return map;
    }
    map = map->next;
  }
  if (c != NULL) *c = '.';
  return NULL;
}

static HDF *var_lookup_obj (CSPARSE *parse, char *name)
{
  CS_LOCAL_MAP *map;
  char *c;

  map = lookup_map (parse, name, &c);
  if (map && map->type == CS_TYPE_VAR)
  {
    if (c == NULL)
    {
      return map->value.h;
    }
    else
    {
      return hdf_get_obj (map->value.h, c+1);
    }
  }
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
  CS_LOCAL_MAP *map;
  char *c;

  map = lookup_map (parse, name, &c);
  if (map) 
  {
    if (map->type == CS_TYPE_VAR)
    {
      if (c == NULL)
      {
	return hdf_obj_value (map->value.h);
      }
      else
      {
	return hdf_get_value (map->value.h, c+1, NULL);
      }
    }
    /* Hmm, if c != NULL, they are asking for a sub member of something
     * which isn't a var... right now we ignore them, I don't know what
     * the right thing is */
    /* hmm, its possible now that they are getting a reference to a
     * string that will be deleted... where is it used? */
    else if (map->type & (CS_TYPE_STRING | CS_TYPE_STRING_ALLOC))
    {
      return map->value.s;
    }
    else if (map->type == CS_TYPE_NUM)
    {
      static char buf[40];
      snprintf (buf, sizeof(buf), "%ld", map->value.n);
      return buf;
    }
  }
  return hdf_get_value (parse->hdf, name, NULL);
}

long int var_int_lookup (CSPARSE *parse, char *name)
{
  char *vs;

  vs = var_lookup (parse, name);

  if (vs == NULL)
    return 0;
  else
    return atoi(vs);
}

typedef struct _token
{
  CSTOKEN_TYPE type;
  char *value;
  size_t len;
} CSTOKEN;

struct _simple_tokens
{
  BOOL two_chars;
  char *token;
  CSTOKEN_TYPE type;
} SimpleTokens[] = {
  { TRUE, "<=", CS_OP_LTE },
  { TRUE, ">=", CS_OP_GTE },
  { TRUE, "==", CS_OP_EQUAL },
  { TRUE, "!=", CS_OP_NEQUAL },
  { TRUE, "||", CS_OP_OR },
  { TRUE, "&&", CS_OP_AND },
  { FALSE, "<", CS_OP_LT },
  { FALSE, ">", CS_OP_GT },
  { FALSE, "!", CS_OP_NOT },
  { FALSE, "+", CS_OP_ADD },
  { FALSE, "-", CS_OP_SUB },
  { FALSE, "*", CS_OP_MULT },
  { FALSE, "/", CS_OP_DIV },
  { FALSE, "%", CS_OP_MOD },
  { FALSE, NULL, 0 }
};

#define MAX_TOKENS 256

static NEOERR *parse_tokens (CSPARSE *parse, char *arg, CSTOKEN *tokens, 
    int *used_tokens)
{
  char tmp[256];
  int ntokens = 0;
  int x;
  BOOL found;
  char *p;
  char *expr = arg;

  while (arg && *arg != '\0')
  {
    while (*arg && isspace(*arg)) arg++;
    if (*arg == '\0') break;
    x = 0;
    found = FALSE;
    while ((found == FALSE) && SimpleTokens[x].token)
    {
      if (((SimpleTokens[x].two_chars == TRUE) && 
	    (*arg == SimpleTokens[x].token[0]) &&
	    (*(arg + 1) == SimpleTokens[x].token[1])) ||
	  ((SimpleTokens[x].two_chars == FALSE) && 
	   (*arg == SimpleTokens[x].token[0])))
      {
	tokens[ntokens++].type = SimpleTokens[x].type;
	found = TRUE;
	arg++;
	if (SimpleTokens[x].two_chars) arg++;
      }
      x++;
    }
    
    if (found == FALSE)
    {
      if (*arg == '#')
      {
	arg++;
	tokens[ntokens].type = CS_TYPE_NUM;
	tokens[ntokens].value = arg;
	strtol(arg, &p, 0);
	if (p == arg)
	{
	  tokens[ntokens].type = CS_TYPE_VAR_NUM;
	  p = strpbrk(arg, "\"?<>=!#-+|&,)*/%[]( \t\r\n");
	  if (p == arg)
	    return nerr_raise (NERR_PARSE, "%s Missing arg after #: %s",
		find_context(parse, -1, tmp, sizeof(tmp)), arg);
	}
	if (p == NULL)
	  tokens[ntokens].len = strlen(arg);
	else
	  tokens[ntokens].len = p - arg;
	ntokens++;
	arg = p;
      }
      else if (*arg == '"')
      {
	arg++;
	tokens[ntokens].type = CS_TYPE_STRING;
	tokens[ntokens].value = arg;
	p = strchr (arg, '"');
	if (p == NULL)
	  return nerr_raise (NERR_PARSE, "%s Missing end of string: %s", 
	      find_context(parse, -1, tmp, sizeof(tmp)), arg);
	tokens[ntokens].len = p - arg;
	ntokens++;
	arg = p + 1;
      }
      else
      {
	tokens[ntokens].type = CS_TYPE_VAR;
	tokens[ntokens].value = arg;
	p = strpbrk(arg, "\"?<>=!#-+|&,)*/%[]( \t\r\n");
	if (p == arg)
	  return nerr_raise (NERR_PARSE, 
	      "%s Var arg specified with no varname: %s",
	      find_context(parse, -1, tmp, sizeof(tmp)), arg);
	if (p == NULL)
	  tokens[ntokens].len = strlen(arg);
	else
	  tokens[ntokens].len = p - arg;
	ntokens++;
	arg = p;
      }
    }
    if (ntokens >= MAX_TOKENS)
	return nerr_raise (NERR_PARSE, 
	    "%s Expression exceeds maximum number of tokens of %d: %s",
	    find_context(parse, -1, tmp, sizeof(tmp)), MAX_TOKENS, expr);
  }
  *used_tokens = ntokens;
  return STATUS_OK;
}

CSTOKEN_TYPE BinaryOpOrder[] = {
   CS_OP_OR,
   CS_OP_AND,
   CS_OP_EQUAL | CS_OP_NEQUAL,
   CS_OP_GT | CS_OP_GTE | CS_OP_LT | CS_OP_LTE,
   CS_OP_ADD | CS_OP_SUB, 
   CS_OP_MULT | CS_OP_DIV | CS_OP_MOD,
   0
};

static NEOERR *parse_expr2 (CSPARSE *parse, CSTOKEN *tokens, int ntokens, 
    CSARG *arg)
{
  NEOERR *err;
  char tmp[256];
  int x, op = 0;

  if (ntokens == 1 || (ntokens == 2 && tokens[0].type == CS_OP_NOT))
  {
    x = 0;
    if (tokens[0].type == CS_OP_NOT) x = 1;
    if (tokens[x].type & CS_TYPES)
    {
      arg->s = tokens[x].value;
      if (tokens[x].len >= 0)
	arg->s[tokens[x].len] = '\0';
      arg->op_type = tokens[x].type;

      if (tokens[x].type == CS_TYPE_NUM)
	arg->n = strtol(arg->s, NULL, 0);
      if (tokens[0].type == CS_OP_NOT) arg->op_type |= CS_OP_NOT;
      return STATUS_OK;
    }
    else
    {
      return nerr_raise (NERR_PARSE, 
	  "%s Terminal token is not an argument, type is %d",
	  find_context(parse, -1, tmp, sizeof(tmp)), tokens[0].type);
    }
  }

  while (BinaryOpOrder[op])
  {
    x = ntokens-1;
    while (x >= 0)
    {
      if (tokens[x].type & BinaryOpOrder[op])
      {
	arg->op_type = tokens[x].type;
	arg->expr2 = (CSARG *) calloc (1, sizeof (CSARG));
	arg->expr1 = (CSARG *) calloc (1, sizeof (CSARG));
	if (arg->expr1 == NULL || arg->expr2 == NULL)
	  return nerr_raise (NERR_NOMEM, 
	      "%s Unable to allocate memory for expression", 
	      find_context(parse, -1, tmp, sizeof(tmp)));
	err = parse_expr2(parse, tokens + x + 1, ntokens-x-1, arg->expr2);
	if (err) return nerr_pass (err);
	err = parse_expr2(parse, tokens, x, arg->expr1);
	if (err) return nerr_pass (err);
	return STATUS_OK;
      }
      x--;
    }
    op++;
  }
  return nerr_raise (NERR_PARSE, "%s Bad Expression",
      find_context(parse, -1, tmp, sizeof(tmp)));
}

static NEOERR *parse_expr (CSPARSE *parse, char *arg, CSARG *expr)
{
  NEOERR *err;
  CSTOKEN tokens[MAX_TOKENS];
  int ntokens = 0;

  memset(tokens, 0, sizeof(CSTOKEN) * MAX_TOKENS);
  err = parse_tokens (parse, arg, tokens, &ntokens);
  if (err) return nerr_pass(err);
  err = parse_expr2 (parse, tokens, ntokens, expr);
  if (err) return nerr_pass(err);
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
  node->arg1.op_type = CS_TYPE_STRING;
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

  node->arg1.op_type = CS_TYPE_VAR;
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

  if (node->arg1.op_type == CS_TYPE_VAR && node->arg1.s != NULL)
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

  /* ne_warn ("var: %s", arg); */
  err = alloc_node (&node);
  if (err) return nerr_pass(err);
  node->cmd = cmd;
  if (arg[0] == '!')
    node->flags |= CSF_REQUIRED;
  arg++;
  /* Validate arg is a var (regex /^[#" ]$/) */
  err = parse_expr (parse, arg, &(node->arg1));
  if (err)
  {
    dealloc_node(&node);
    return nerr_pass(err);
  }

  *(parse->next) = node;
  parse->next = &(node->next);
  parse->current = node;
  
  return STATUS_OK;
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

  err = hdf_get_copy (parse->hdf, a, &s, NULL);
  if (err) 
  {
    dealloc_node(&node);
    return nerr_pass (err);
  }
  if (node->flags & CSF_REQUIRED && s == NULL) 
  {
    dealloc_node(&node);
    return nerr_raise (NERR_NOT_FOUND, "%s Unable to evar empty variable %s",
	find_context(parse, -1, tmp, sizeof(tmp)), a);
  }

  node->arg1.op_type = CS_TYPE_VAR;
  node->arg1.s = a;
  *(parse->next) = node;
  parse->next = &(node->next);
  parse->current = node;

  save_context = parse->context;
  save_infile = parse->in_file;
  parse->context = a;
  parse->in_file = 0;
  if (s) err = cs_parse_string (parse, s, strlen(s));
  parse->context = save_context;
  parse->in_file = save_infile;

  return nerr_pass (err);
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

  err = parse_expr (parse, arg, &(node->arg1));
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

char *arg_eval (CSPARSE *parse, CSARG *arg)
{
  if (arg->op_type & CS_OP_NOT)
  {
    ne_warn ("String eval shouldn't have a NOT op");
    return NULL;
  }
  else
  {
    switch ((arg->op_type & CS_TYPES))
    {
      case CS_TYPE_STRING:
      case CS_TYPE_STRING_ALLOC:
	return arg->s;
      case CS_TYPE_VAR:
	return var_lookup (parse, arg->s);
      case CS_TYPE_NUM:
      case CS_TYPE_VAR_NUM:
	/* These types should force numeric evaluation... so nothing
	 * should get here */
      default:
	ne_warn ("Unsupported type %s in arg_eval", arg->op_type);
	return NULL;
    }
  }
}

long int arg_eval_num (CSPARSE *parse, CSARG *arg)
{
  long int v = 0;

  switch ((arg->op_type & CS_TYPES))
  {
    case CS_TYPE_STRING:
    case CS_TYPE_STRING_ALLOC:
      v = strtol(arg->s, NULL, 0);
      break;
    case CS_TYPE_NUM:
      v = arg->n;
      break;

    case CS_TYPE_VAR:
    case CS_TYPE_VAR_NUM:
      v = var_int_lookup (parse, arg->s);
      break;
    default:
      ne_warn ("Unsupported type %s in arg_eval_num", arg->op_type);
      v = 0;
      break;
  }
  if (arg->op_type & CS_OP_NOT)
    v = !v;
  return v;
}

static NEOERR *eval_expr (CSPARSE *parse, CSARG *expr, CSARG *result)
{
  memset(result, 0, sizeof(CSARG));
  if (expr->op_type & CS_TYPES)
  {
    *result = *expr;
    /* we transfer ownership of the string here.. ugh */
    if (expr->op_type == CS_TYPE_STRING_ALLOC)
      expr->op_type = CS_TYPE_STRING;
    return STATUS_OK;
  }
  else
  {
    CSARG arg1, arg2;
    NEOERR *err;

    err = eval_expr (parse, expr->expr1, &arg1);
    if (err) return nerr_pass(err);
    err = eval_expr (parse, expr->expr2, &arg2);
    if (err) return nerr_pass(err);
    /* Check for type conversion */
    if ((arg1.op_type & (CS_OP_NOT | CS_TYPE_NUM | CS_TYPE_VAR_NUM)) ||
	(arg2.op_type & (CS_OP_NOT | CS_TYPE_NUM | CS_TYPE_VAR_NUM)) ||
	(expr->op_type & (CS_OP_AND | CS_OP_OR | CS_OP_SUB | CS_OP_MULT | CS_OP_DIV | CS_OP_MOD)))
    {
      /* eval as num */
      long int n1, n2;

      result->op_type = CS_TYPE_NUM;
      n1 = arg_eval_num (parse, &arg1);
      n2 = arg_eval_num (parse, &arg2);

      switch (expr->op_type)
      {
	case CS_OP_EQUAL:
	  result->n = (n1 == n2) ? 1 : 0;
	  break;
	case CS_OP_NEQUAL:
	  result->n = (n1 != n2) ? 1 : 0;
	  break;
	case CS_OP_LT:
	  result->n = (n1 < n2) ? 1 : 0;
	  break;
	case CS_OP_LTE:
	  result->n = (n1 <= n2) ? 1 : 0;
	  break;
	case CS_OP_GT:
	  result->n = (n1 > n2) ? 1 : 0;
	  break;
	case CS_OP_GTE:
	  result->n = (n1 >= n2) ? 1 : 0;
	  break;
	case CS_OP_AND:
	  result->n = (n1 && n2) ? 1 : 0;
	  break;
	case CS_OP_OR:
	  result->n = (n1 || n2) ? 1 : 0;
	  break;
	case CS_OP_ADD:
	  result->n = (n1 + n2);
	  break;
	case CS_OP_SUB:
	  result->n = (n1 - n2);
	  break;
	case CS_OP_MULT:
	  result->n = (n1 * n2);
	  break;
	case CS_OP_DIV:
	  if (n2 == 0) result->n = UINT_MAX;
	  else result->n = (n1 / n2);
	  break;
	case CS_OP_MOD:
	  if (n2 == 0) result->n = 0;
	  else result->n = (n1 % n2);
	  break;
	default:
	  ne_warn ("Unsupported op %d in eval_expr", expr->op_type);
	  break;
      }
    }
    else /* eval as string */
    {
      char *s1, *s2;
      int out;

      result->op_type = CS_TYPE_NUM;
      s1 = arg_eval (parse, &arg1);
      s2 = arg_eval (parse, &arg2);

      if ((s1 == NULL) || (s2 == NULL))
      {
	switch (expr->op_type)
	{
	  case CS_OP_EQUAL:
	    result->n = (s1 == s2) ? 1 : 0;
	    break;
	  case CS_OP_NEQUAL:
	    result->n = (s1 != s2) ? 1 : 0;
	    break;
	  case CS_OP_LT:
	    result->n = ((s1 == NULL) && (s2 != NULL)) ? 1 : 0;
	    break;
	  case CS_OP_LTE:
	    result->n = (s1 == NULL) ? 1 : 0;
	    break;
	  case CS_OP_GT:
	    result->n = ((s1 != NULL) && (s2 == NULL)) ? 1 : 0;
	    break;
	  case CS_OP_GTE:
	    result->n = (s2 == NULL) ? 1 : 0;
	    break;
	  case CS_OP_ADD:
	    if (s1 == NULL) 
	    {
	      result->s = s2;
	      result->op_type = arg2.op_type;
	      arg2.op_type = CS_TYPE_STRING;
	    }
	    else
	    {
	      result->s = s1;
	      result->op_type = arg1.op_type;
	      arg1.op_type = CS_TYPE_STRING;
	    }
	    break;
	  default:
	    ne_warn ("Unsupported op %d in eval_expr", expr->op_type);
	    break;
	}
      }
      else
      {
	out = strcmp (s1, s2);
	switch (expr->op_type)
	{
	  case CS_OP_EQUAL:
	    result->n = (!out) ? 1 : 0;
	    break;
	  case CS_OP_NEQUAL:
	    result->n = (out) ? 1 : 0;
	    break;
	  case CS_OP_LT:
	    result->n = (out < 0) ? 1 : 0;
	    break;
	  case CS_OP_LTE:
	    result->n = (out <= 0) ? 1 : 0;
	    break;
	  case CS_OP_GT:
	    result->n = (out > 0) ? 1 : 0;
	    break;
	  case CS_OP_GTE:
	    result->n = (out >= 0) ? 1 : 0;
	    break;
	  case CS_OP_ADD:
	    result->op_type = CS_TYPE_STRING_ALLOC;
	    result->s = (char *) calloc ((strlen(s1) + strlen(s2) + 1), sizeof(char));
	    if (result->s == NULL)
	      return nerr_raise (NERR_NOMEM, "Unable to allocate memory to concatenate strings in expression: %s + %s", s1, s2);
	    strcpy(result->s, s1);
	    strcat(result->s, s2);
	    break;
	  default:
	    ne_warn ("Unsupported op %d in expr_eval", expr->op_type);
	    break;
	}
      }
    }
    
    if (arg1.op_type == CS_TYPE_STRING_ALLOC)
      free(arg1.s);
    if (arg2.op_type == CS_TYPE_STRING_ALLOC)
      free(arg2.s);
  }
  return STATUS_OK;
}

static NEOERR *var_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  NEOERR *err = STATUS_OK;
  CSARG val;

  err = eval_expr(parse, &(node->arg1), &val);
  if (err) return nerr_pass(err);
  if (val.op_type & (CS_TYPE_NUM | CS_TYPE_VAR_NUM))
  { 
    char buf[256];
    long int n_val;

    n_val = arg_eval_num (parse, &val);
    snprintf (buf, sizeof(buf), "%ld", n_val);
    err = parse->output_cb (parse->output_ctx, buf);
  }
  else
  {
    char *s = arg_eval (parse, &val);
    /* Do we set it to blank if s == NULL? */
    if (s)
    {
      err = parse->output_cb (parse->output_ctx, s);
    }
  }
  if (val.op_type == CS_TYPE_STRING_ALLOC)
    free(val.s);

  *next = node->next;
  return nerr_pass(err);
}


static NEOERR *if_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  NEOERR *err = STATUS_OK;
  int eval_true = 0;
  CSARG val;

  err = eval_expr(parse, &(node->arg1), &val);
  if (err) return nerr_pass (err);
  if (val.op_type & (CS_TYPE_NUM | CS_TYPE_VAR_NUM))
    eval_true = arg_eval_num (parse, &val);
  else
  {
    char *s;
    BOOL not = FALSE;
    if (val.op_type & CS_OP_NOT)
    {
      not = TRUE;
      val.op_type &= ~CS_OP_NOT;
    }
    s = arg_eval (parse, &val);
    if (s == NULL || *s == '\0')
      eval_true = 0;
    else
      eval_true = 1;

    if (not == TRUE)
      eval_true = !eval_true;
  }
  if (val.op_type == CS_TYPE_STRING_ALLOC)
    free(val.s);

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
  node->arg1.op_type = CS_TYPE_VAR;
  node->arg1.s = lvar;

  node->arg2.op_type = CS_TYPE_VAR;
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
    each_map.type = CS_TYPE_VAR;
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
    node->arg1.op_type = CS_TYPE_STRING;
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
    node->arg1.op_type = CS_TYPE_VAR;
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
  char *a = NULL, *p = NULL, *s;
  char tmp[256];
  char name[256];
  int x = 0;
  BOOL last = FALSE;

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
    while (*s && isspace(*s)) s++;
    a = strpbrk(s, ",)");
    if (a == NULL)
    {
      err = nerr_raise (NERR_PARSE, 
	  "%s Missing right paren in def %s",
	  find_context(parse, -1, tmp, sizeof(tmp)), arg);
      break;
    }
    if (*a == ')') last = TRUE;
    *a = '\0';
    /* cut out ending whitespace */
    p = strpbrk(s, " \t\r\n");
    if (p != NULL) *p = '\0';
    p = strpbrk(s, "\"?<>=!#-+|&,)*/%[]( \t\r\n");
    if (p != NULL)
    {
      err = nerr_raise (NERR_PARSE, 
	  "%s Invalid character in def %s argument: %c",
	  find_context(parse, -1, tmp, sizeof(tmp)), arg, *p);
      break;
    }
    carg->s = s;
    if (last == TRUE) break;
    s = a+1;
  }
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
  BOOL last = FALSE;

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
  node->arg1.op_type = CS_TYPE_MACRO;
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
    a = strpbrk(s, ",)");
    if (a == NULL)
    {
      err = nerr_raise (NERR_PARSE, 
	  "%s Missing right paren in def %s",
	  find_context(parse, -1, tmp, sizeof(tmp)), arg);
      break;
    }
    if (*a == ')') last = TRUE;
    *a = '\0';
    err = parse_expr (parse, s, carg);
    if (err) break;
    if (last == TRUE) break;
    s = a+1;
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
    CSARG val;
    map = &call_map[x];
    if (x) call_map[x-1].next = map;

    map->name = darg->s;
    err = eval_expr(parse, carg, &val);
    if (err) break;
    if (val.op_type & (CS_TYPE_STRING | CS_TYPE_STRING_ALLOC))
    {
      map->value.s = val.s;
      map->type = val.op_type;
    }
    else if (val.op_type & CS_TYPE_NUM)
    {
      map->value.n = val.n;
      map->type = CS_TYPE_NUM;
    }
    else if (val.op_type & (CS_TYPE_VAR | CS_TYPE_VAR_NUM))
    {
      var = var_lookup_obj (parse, val.s);
      map->value.h = var;
      map->type = CS_TYPE_VAR;
    }
    else
    {
      ne_warn("Unsupported type %d in call_expr", val.op_type);
    }
    map->next = parse->locals;

    darg = darg->next;
    carg = carg->next;
  }

  if (err == STATUS_OK)
  {
    map = parse->locals;
    if (macro->n_args) parse->locals = call_map;
    err = render_node (parse, macro->tree->case_0);
    parse->locals = map;
  }
  for (x = 0; x < macro->n_args; x++)
  {
    if (call_map[x].type == CS_TYPE_STRING_ALLOC)
    {
      free(call_map[x].value.s);
    }
  }
  free (call_map);

  *next = node->next;
  return nerr_pass(err);
}

static NEOERR *set_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  CSTREE *node;
  char *s;
  char tmp[256];
  char name[256];
  int x = 0;

  err = alloc_node (&node);
  if (err) return nerr_pass(err);
  node->cmd = cmd;
  arg++;
  s = arg;
  while (*s && isspace(*s)) s++;
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
  node->arg1.op_type = CS_TYPE_VAR;
  node->arg1.s = strdup(name);

  err = parse_expr(parse, s, &(node->arg2));
  if (err)
  {
    dealloc_node(&node);
    return nerr_pass(err);
  }

  *(parse->next) = node;
  parse->next = &(node->next);
  parse->current = node;

  return STATUS_OK;
}

static NEOERR *set_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  NEOERR *err = STATUS_OK;
  CSARG val;

  err = eval_expr(parse, &(node->arg2), &val);
  if (err) return nerr_pass (err);

  if (val.op_type & (CS_TYPE_NUM | CS_TYPE_VAR_NUM))
  { 
    char buf[256];
    long int n_val;

    n_val = arg_eval_num (parse, &val);
    snprintf (buf, sizeof(buf), "%ld", n_val);
    err = var_set_value (parse, node->arg1.s, buf);
  }
  else
  {
    char *s = arg_eval (parse, &val);
    /* Do we set it to blank if s == NULL? */
    if (s)
    {
      err = var_set_value (parse, node->arg1.s, s);
    }
  }
  if (val.op_type == CS_TYPE_STRING_ALLOC)
    free(val.s);

  *next = node->next;
  return nerr_pass (err);
}

static NEOERR *loop_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  CSTREE *node;
  CSARG *carg, *larg = NULL;
  BOOL last = FALSE;
  char *lvar;
  char *p, *a;
  char tmp[256];
  int x;

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
	"%s Improperly formatted loop directive: %s", 
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
	  "%s Improperly formatted loop directive: %s", 
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
	"%s Improperly formatted loop directive: %s", 
	find_context(parse, -1, tmp, sizeof(tmp)), arg);
  }
  node->arg1.op_type = CS_TYPE_VAR;
  node->arg1.s = lvar;

  x = 0;
  while (*p)
  {
    carg = (CSARG *) calloc (1, sizeof(CSARG));
    if (carg == NULL)
    {
      err = nerr_raise (NERR_NOMEM, 
	  "%s Unable to allocate memory for CSARG in loop %s",
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
    a = strpbrk(p, ",");
    if (a == NULL) last = TRUE;
    else *a = '\0';
    err = parse_expr (parse, p, carg);
    if (err) break;
    if (last == TRUE) break;
    p = a+1;
  }
  if (!err && ((x < 1) || (x > 3)))
  {
    err = nerr_raise (NERR_PARSE, 
	"%s Incorrect number of arguments, expected 1, 2, or 3 got %d in loop: %s",
	find_context(parse, -1, tmp, sizeof(tmp)), x, arg);
  }

  /* ne_warn ("loop %s %s", lvar, p); */

  *(parse->next) = node;
  parse->next = &(node->case_0);
  parse->current = node;

  return STATUS_OK;
}

static NEOERR *loop_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  NEOERR *err = STATUS_OK;
  CS_LOCAL_MAP each_map;
  int var;
  int start = 0, end = 0, step = 1;
  int x, iter = 1;
  CSARG *carg;
  CSARG val;

  carg = node->vargs;
  if (carg == NULL) return nerr_raise (NERR_ASSERT, "No arguments in loop eval?");
  err = eval_expr(parse, carg, &val);
  if (err) return nerr_pass(err);
  end = arg_eval_num(parse, &val);
  if (val.op_type == CS_TYPE_STRING_ALLOC) free(val.s);
  if (carg->next)
  {
    start = end;
    carg = carg->next;
    err = eval_expr(parse, carg, &val);
    if (err) return nerr_pass(err);
    end = arg_eval_num(parse, &val);
    if (val.op_type == CS_TYPE_STRING_ALLOC) free(val.s);
    if (carg->next)
    {
      carg = carg->next;
      err = eval_expr(parse, carg, &val);
      if (err) return nerr_pass(err);
      step = arg_eval_num(parse, &val);
      if (val.op_type == CS_TYPE_STRING_ALLOC) free(val.s);
    }
  }
  /* automatically handle cases where the step is backwards */
  if (((step < 0) && (start < end)) || 
      ((step > 0) && (end < start)))
  {
    x = start;
    start = end;
    end = x;
  }
  if (step == 0)
  {
    if (start == end) iter = 1;
    else iter = 0;
  }
  else 
  {
    iter = abs((end - start) / step + 1);
  }

  if (iter > 0)
  {
    /* Init and install local map */
    each_map.type = CS_TYPE_NUM;
    each_map.name = node->arg1.s;
    each_map.next = parse->locals;
    parse->locals = &each_map;

    var = start;
    for (x = 0, var = start; x < iter; x++, var += step)
    {
      each_map.value.n = var;
      err = render_node (parse, node->case_0);
      if (err != STATUS_OK) break;
    } 

    /* Remove local map */
    parse->locals = each_map.next;
  }

  *next = node->next;
  return nerr_pass (err);
}
static NEOERR *endloop_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  STACK_ENTRY *entry;

  err = uListGet (parse->stack, -1, (void **)&entry);
  if (err != STATUS_OK) return nerr_pass(err);

  parse->next = &(entry->tree->next);
  parse->current = entry->tree;
  return STATUS_OK;
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

static NEOERR *dump_node (CSPARSE *parse, CSTREE *node, int depth, void *ctx, 
    CSOUTFUNC cb, char *buf, int blen)
{
  NEOERR *err;

  while (node != NULL)
  {
    snprintf (buf, blen, "%*s %s ", depth, "", Commands[node->cmd].cmd);
    err = cb (ctx, buf);
    if (err) return nerr_pass (err);
    if (node->cmd)
    {
      if (node->arg1.op_type)
      {
	if (node->arg1.op_type == CS_TYPE_NUM)
	{
	  snprintf (buf, blen, "%ld ", node->arg1.n);
	}
	else if (node->arg1.op_type == CS_TYPE_MACRO)
	{
	  snprintf (buf, blen, "%s ", node->arg1.macro->name);
	}
	else 
	{
	  snprintf (buf, blen, "%s ", node->arg1.s);
	}
	err = cb (ctx, buf);
	if (err) return nerr_pass (err);
      }
      if (node->arg2.op_type)
      {
	if (node->arg2.op_type == CS_TYPE_NUM)
	{
	  snprintf (buf, blen, "%ld", node->arg2.n);
	}
	else 
	{
	  snprintf (buf, blen, "%s", node->arg2.s);
	}
	err = cb (ctx, buf);
	if (err) return nerr_pass (err);
      }
      if (node->vargs)
      {
	CSARG *arg;
	arg = node->vargs;
	while (arg)
	{
	  if (arg->op_type == CS_TYPE_NUM)
	  {
	    snprintf (buf, blen, "%ld ", arg->n);
	  }
	  else 
	  {
	    snprintf (buf, blen, "%s ", arg->s);
	  }
	  err = cb (ctx, buf);
	  if (err) return nerr_pass (err);
	  arg = arg->next;
	}
      }
    }
    err = cb (ctx, "\n");
    if (err) return nerr_pass (err);
    if (node->case_0)
    {
      snprintf (buf, blen, "%*s %s\n", depth, "", "Case 0");
      err = cb (ctx, buf);
      if (err) return nerr_pass (err);
      err = dump_node (parse, node->case_0, depth+1, ctx, cb, buf, blen);
      if (err) return nerr_pass (err);
    }
    if (node->case_1)
    {
      snprintf (buf, blen, "%*s %s\n", depth, "", "Case 1");
      err = cb (ctx, buf);
      if (err) return nerr_pass (err);
      err = dump_node (parse, node->case_1, depth+1, ctx, cb, buf, blen);
      if (err) return nerr_pass (err);
    }
    node = node->next;
  }
  return STATUS_OK;
}

NEOERR *cs_dump (CSPARSE *parse, void *ctx, CSOUTFUNC cb)
{
  CSTREE *node;
  char buf[4096];

  if (parse->tree == NULL)
    return nerr_raise (NERR_ASSERT, "No parse tree exists");

  node = parse->tree;
  return nerr_pass (dump_node (parse, node, 0, ctx, cb, buf, sizeof(buf)));
}

#if 0
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
    fprintf (fp, "\n\t  { %d, %s, %ld }, ", node->arg1.op_type, s, node->arg1.n);
    free(s);
    s = repr_string_alloc (node->arg2.s);
    if (s == NULL)
      return nerr_raise(NERR_NOMEM, "Unable to allocate space for repr");
    fprintf (fp, "\n\t  { %d, %s, %ld }, ", node->arg2.op_type, s, node->arg2.n);
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
#endif

