/*
 * Copyright (C) 2000 Brandon Long
 *
 */

/*
 * CS Syntax
 * ------------------------------------------------------------------
 * CS          := (ANYTHING COMMAND)*
 * CS_OPEN     := <?cs
 * CS_CLOSE    := ?>
 * COMMAND     := (CMD_IF | CMD_VAR | CMD_EVAR | CMD_INCLUDE | CMD_EACH
 *                 | CMD_DEF | CMD_CALL )
 * CMD_IF      := CS_OPEN IF CS_CLOSE CS CMD_ENDIF
 * CMD_ENDIF   := CS_OPEN ENDIF CS_CLOSE
 * CMD_INCLUDE := CS_OPEN INCLUDE CS_CLOSE
 * CMD_DEF     := CS_OPEN DEF CS_CLOSE
 * CMD_CALL    := CS_OPEN CALL CS_CLOSE
 * CALL        := call:VAR LPAREN ARG (,ARG)* RPAREN
 * DEF         := def:VAR LPAREN ARG (,ARG)* RPAREN
 * INCLUDE     := include:(VAR|STRING)
 * IF          := (if:ARG OP ARG|if:ARG|if:!ARG)
 * ENDIF       := /if
 * OP          := ( == | != | < | <= | > | >= | || | &&)
 * ARG         := (STRING|VAR|NUM)
 * STRING      := "[^"]"
 * VAR         := [^"<> ]+
 * NUM         := #[0-9]+
 */

#ifndef _CSHDF_H_
#define _CSHDF_H_ 1

#include "util/neo_err.h"
#include "util/ulist.h"
#include "util/neo_hdf.h"

typedef enum
{
  CS_TYPE_STRING = 1,
  CS_TYPE_NUM,
  CS_TYPE_VAR,
  CS_TYPE_VAR_NUM,
  CS_TYPE_MACRO
} CSARG_TYPE;

typedef enum
{
  /* Unary operators */
  CS_OP_EXISTS = 1,
  CS_OP_NOT,

  /* Binary Operators */
  CS_OP_EQUAL,
  CS_OP_NEQUAL,
  CS_OP_LT,
  CS_OP_LTE,
  CS_OP_GT,
  CS_OP_GTE,
  CS_OP_AND,
  CS_OP_OR

} CS_OP;

typedef struct _arg
{
  CSARG_TYPE type;
  char *s;
  long int n;
  struct _macro *macro;
  struct _arg *next;
} CSARG;

#define CSF_REQUIRED (1<<0)

/* WARNING: If you change this struct, you have to modify dump_node_c
 * in csparse.c to dump the new struct
 */
typedef struct _tree 
{
  int node_num;
  int cmd;
  int flags;
  CSARG arg1;
  CSARG arg2;
  CSARG *vargs;
  CS_OP op;

  struct _tree *case_0;
  struct _tree *case_1;
  struct _tree *next;
} CSTREE;

typedef NEOERR* (*CSOUTFUNC)(void *, char *);

typedef struct _local_map
{
  int is_map;
  char *name;
  union
  {
    char *s;
    long int n;
    HDF *h;
  } value;
  struct _local_map *next;
} CS_LOCAL_MAP;

typedef struct _macro
{
  char *name;
  int n_args;
  CSARG *args;

  CSTREE *tree;

  struct _macro *next;
} CS_MACRO;

typedef struct _parse
{
  char *context;         /* A string identifying where the parser is parsing */
  int in_file;           /* Indicates if current context is a file */
  int offset;

  ULIST *stack;
  CSTREE *tree;
  CSTREE *current;
  CSTREE **next;

  HDF *hdf;

  CS_LOCAL_MAP *locals;
  CS_MACRO *macros;

  /* Output */
  void *output_ctx;
  CSOUTFUNC output_cb;

} CSPARSE;

NEOERR *cs_init (CSPARSE **parse, HDF *hdf);
NEOERR *cs_parse_file (CSPARSE *parse, char *path);
NEOERR *cs_parse_string (CSPARSE *parse, char *buf, size_t blen);
NEOERR *cs_render (CSPARSE *parse, void *ctx, CSOUTFUNC cb);
NEOERR *cs_dump (CSPARSE *parse);
NEOERR *cs_dump_c (CSPARSE *parse, char *path);
void cs_destroy (CSPARSE **parse);

#endif /* _CSHDF_H_ */
