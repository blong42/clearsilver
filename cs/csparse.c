
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "neo_err.h"
#include "neo_misc.h"
#include "neo_str.h"
#include "ulist.h"
#include "cs.h"

typedef enum
{
  ST_SAME = 0,
  ST_GLOBAL = 1<<0,
  ST_IF = 1<<1,
  ST_ELSE = 1<<2,
  ST_EACH = 1<<3,
  ST_POP = 1<<4,
} CS_STATE;

#define ST_ANYWHERE (ST_EACH | ST_ELSE | ST_IF | ST_GLOBAL)

typedef struct _stack_entry 
{
  int state;
  CSTREE *tree;
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
static NEOERR *evar_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *if_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *if_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *else_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *else_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *elif_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *elif_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *endif_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *endif_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *each_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *each_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *endeach_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *endeach_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);
static NEOERR *include_parse (CSPARSE *parse, int cmd, char *arg);
static NEOERR *include_eval (CSPARSE *parse, CSTREE *node, CSTREE **next);

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
    evar_parse, evar_eval,    1},
  {"if",      sizeof("if")-1,      ST_ANYWHERE,     ST_IF,   
    if_parse, if_eval,      1},
  {"else",    sizeof("else")-1,    ST_IF,           ST_POP | ST_ELSE, 
    else_parse, else_eval,    0},
  {"elseif",  sizeof("elseif")-1,  ST_IF,           ST_SAME, 
    elif_parse, elif_eval,   1},
  {"elif",    sizeof("elif")-1,    ST_IF,           ST_SAME, 
    elif_parse, elif_eval,   1},
  {"/if",     sizeof("/if")-1,     ST_IF | ST_ELSE, ST_POP,  
    endif_parse, endif_eval,   0},
  {"each",    sizeof("each")-1,    ST_ANYWHERE,     ST_EACH, 
    each_parse, each_eval,    1},
  {"/each",   sizeof("/each")-1,   ST_EACH,         ST_POP,  
    endeach_parse, endeach_eval, 0},
  {"include", sizeof("include")-1, ST_ANYWHERE,     ST_SAME, 
    include_parse, include_eval, 1},
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
  if (*node == NULL) return;
  free(*node);
  *node = NULL;
  return;
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

  entry = (STACK_ENTRY *) malloc (sizeof (STACK_ENTRY));
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

  /* Need to walk and dealloc parse tree */

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
	      parse->current = entry->tree;
	      free(entry);
	    }
	    if ((Commands[i].next_state & ~ST_POP) != ST_SAME)
	    {
	      entry = (STACK_ENTRY *) malloc (sizeof (STACK_ENTRY));
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

static NEOERR *evar_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  /* An empty node... */
  *next = node->next;
  return STATUS_OK;
}

static NEOERR *parse_arg (CSPARSE *parse, char *buf, CSARG *arg, char **remain)
{
  char *p;
  char tmp[256];

  if (buf[0] == '#')
  {
    arg->type = CS_TYPE_NUM;
    arg->n = strtol(buf+1, remain, 0);
    if (*remain == buf+1)
    {
      arg->type = CS_TYPE_VAR_NUM;
      arg->s = buf+1;
      *remain = strpbrk(buf+1, "?<>=!#-+|& ");
      if (*remain == buf+1)
	return nerr_raise (NERR_PARSE, "%s Missing arg after #: %s",
	    find_context(parse, -1, tmp, sizeof(tmp)), buf);

    }
  }
  else if (buf[0] == '"')
  {
    arg->type = CS_TYPE_STRING;
    arg->s = buf + 1;
    p = strchr (buf, '"');
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
    *remain = strpbrk(buf, "?<>=!#-+|& ");
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
    if (r[x] == '=' && r[x+1] && r[x+1] == '=')
    {
      node->op = CS_OP_EQUAL;
      x+=2;
    }
    if (r[x] == '!' && r[x+1] && r[x+1] == '=')
    {
      node->op = CS_OP_NEQUAL;
      x+=2;
    }
    else if (r[x] == '<' && r[x+1] && r[x+1] == '=')
    {
      node->op = CS_OP_LTE;
      x+=2;
    }
    else if (r[x] == '>' && r[x+1] && r[x+1] == '=')
    {
      node->op = CS_OP_GTE;
      x+=2;
    }
    else if (r[x] == '&' && r[x+1] && r[x+1] == '&')
    {
      node->op = CS_OP_AND;
      x+=2;
    }
    else if (r[x] == '|' && r[x+1] && r[x+1] == '|')
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
	(node->op == CS_OP_OR))
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
	  case CS_OP_AND:
	  case CS_OP_OR:
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

static NEOERR *else_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  *next = node->next;
  return STATUS_OK;
}

static NEOERR *elif_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  STACK_ENTRY *entry;

  /* ne_warn ("elif: %s", arg); */
  err = uListGet (parse->stack, -1, (void **)&entry);
  if (err != STATUS_OK) return nerr_pass(err);

  parse->next = &(entry->tree->case_1);

  return nerr_pass(if_parse(parse, cmd, arg));
}

static NEOERR *elif_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  *next = node->next;
  return STATUS_OK;
}

static NEOERR *endif_parse (CSPARSE *parse, int cmd, char *arg)
{
  NEOERR *err;
  STACK_ENTRY *entry;

  /* ne_warn ("endif"); */
  err = uListGet (parse->stack, -1, (void **)&entry);
  if (err != STATUS_OK) return nerr_pass(err);

  parse->next = &(entry->tree->next);
  parse->current = entry->tree;
  return STATUS_OK;
}

static NEOERR *endif_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  *next = node->next;
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

  var = hdf_get_obj (parse->hdf, node->arg2.s);

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

static NEOERR *endeach_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
{
  *next = node->next;
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

static NEOERR *include_eval (CSPARSE *parse, CSTREE *node, CSTREE **next)
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

