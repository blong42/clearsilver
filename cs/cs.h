/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the FSF's
 * Library Gnu Public License (LGPL).
 *
 * Copyright (C) 2001 by Brandon Long
 */

/*
 * CS Syntax (pseudo BNF, pseudo accurate)
 * ------------------------------------------------------------------
 * CS          := (ANYTHING COMMAND)*
 * CS_OPEN     := <?cs
 * CS_CLOSE    := ?>
 * COMMAND     := (CMD_IF | CMD_VAR | CMD_EVAR | CMD_INCLUDE | CMD_EACH
 *                 | CMD_DEF | CMD_CALL | CMD_SET )
 * CMD_IF      := CS_OPEN IF CS_CLOSE CS CMD_ENDIF
 * CMD_ENDIF   := CS_OPEN ENDIF CS_CLOSE
 * CMD_INCLUDE := CS_OPEN INCLUDE CS_CLOSE
 * CMD_DEF     := CS_OPEN DEF CS_CLOSE
 * CMD_CALL    := CS_OPEN CALL CS_CLOSE
 * CMD_SET     := CS_OPEN SET CS_CLOSE
 * SET         := set:VAR = EXPR
 * EXPR        := (ARG | ARG OP EXPR)
 * CALL        := call:VAR LPAREN ARG (,ARG)* RPAREN
 * DEF         := def:VAR LPAREN ARG (,ARG)* RPAREN
 * INCLUDE     := include:(VAR|STRING)
 * IF          := (if:ARG OP ARG|if:ARG|if:!ARG)
 * ENDIF       := /if
 * OP          := ( == | != | < | <= | > | >= | || | && | + | - | * | / | % )
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
  CS_OP_OR,
  CS_OP_ADD,
  CS_OP_SUB,
  CS_OP_MULT,
  CS_OP_DIV,
  CS_OP_MOD

} CS_OP;

typedef struct _arg
{
  CSARG_TYPE type;
  CS_OP op;
  char *s;
  long int n;
  struct _macro *macro;
  struct _arg *next;
} CSARG;

#define CSF_REQUIRED (1<<0)

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
  ULIST *alloc;
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

/*
 * Function: cs_init - create and initialize a CS context
 * Description: cs_init will create a CSPARSE structure and initialize
 *       it.  This structure maintains the state and information
 *       necessary for parsing and rendering a CS template.
 * Input: parse - a pointer to a pointer to a CSPARSE structure that
 *        will be created
 *        hdf - the HDF dataset to be used during parsing and rendering
 * Output: parse will contain a pointer to the allocated CSPARSE
 *         structure.  This structure will be deallocated with
 *         cs_destroy()
 * Return: NERR_NOMEM
 * MT-Level: cs routines perform no locking, and neither do hdf
 *           routines.  They should be safe in an MT environment as long
 *           as they are confined to a single thread.
 */
NEOERR *cs_init (CSPARSE **parse, HDF *hdf);

/*
 * Function: cs_parse_file - parse a CS template file
 * Description: cs_parse_file will parse the CS template located at
 *              path.  It will use hdf_search_path() if path does not
 *              begin with a '/'.  The parsed CS template will be
 *              appended to the current parse tree stored in the CSPARSE
 *              structure.  The entire file is loaded into memory and
 *              parsed in place.
 * Input: parse - a CSPARSE structure created with cs_init
 *        path - the path to the file to parse
 * Output: None
 * Return: NERR_ASSERT - if path == NULL
 *         NERR_NOT_FOUND - if path isn't found
 *         NERR_SYSTEM - if path can't be accessed
 *         NERR_NOMEM - unable to allocate memory to load file into memory
 *         NERR_PARSE - error in CS template
 */
NEOERR *cs_parse_file (CSPARSE *parse, char *path);

/*
 * Function: cs_parse_string - parse a CS template string
 * Description: cs_parse_string parses a string.  The string is
 *              modified, and internal references are kept by the parse
 *              tree.  For this reason, ownership of the string is
 *              transfered to the CS system, and the string will be
 *              free'd when cs_destroy() is called.
 *              The parse information will be appended to the current
 *              parse tree.  During parse, the only HDF variables which
 *              are evaluated are those used in evar or include
 *              statements.
 * Input: parse - a CSPARSE structure created with cs_init
 *        buf - the string to parse.  Embedded NULLs are not currently
 *              supported
 *        blen - the length of the string
 * Output: None
 * Return: NERR_PARSE - error in CS template
 *         NERR_NOMEM - unable to allocate memory for parse structures
 *         NERR_NOT_FOUND - missing required variable
 */
NEOERR *cs_parse_string (CSPARSE *parse, char *buf, size_t blen);

/*
 * Function: cs_render - render a CS parse tree
 * Description: cs_render will evaluate a CS parse tree, calling the
 *              CSOUTFUNC passed to it for output.  Note that calling
 *              cs_render multiple times on the same parse tree may or
 *              may not render the same output as the set statement has
 *              side-effects, it updates the HDF data used by the
 *              render.  Typically, you will call one of the cs_parse
 *              functions before calling this function.
 * Input: parse - the CSPARSE structure containing the CS parse tree
 *                that will be evaluated
 *        ctx - user data that will be passed as the first variable to
 *              the CSOUTFUNC.
 *        cb - a CSOUTFUNC called to render the output.  A CSOUTFUNC is
 *             defined as:
 *                 typedef NEOERR* (*CSOUTFUNC)(void *, char *);
 * Output: None
 * Return: NERR_NOMEM - Unable to allocate memory for CALL or SET
 *                      functions
 *         any error your callback functions returns
 */
NEOERR *cs_render (CSPARSE *parse, void *ctx, CSOUTFUNC cb);

/*
 * Function: cs_dump - dump the cs parse tree
 * Description: cs_dump will dump the CS parse tree in the parse struct.
 *              This can be useful for debugging your templates.
 *              This function also uses the CSOUTFUNC callback to
 *              display the parse tree.
 * Input: parse - the CSPARSE structure created with cs_init
 *        ctx - user data to be passed to the CSOUTFUNC
 *        cb - a CSOUTFUNC callback
 * Output: None
 * Return: NERR_ASSERT if there is no parse tree
 *         anything your CSOUTFUNC may return
 */
NEOERR *cs_dump (CSPARSE *parse, void *ctx, CSOUTFUNC cb);

/*
 * Function: cs_destroy - clean up and dealloc a parse tree
 * Description: cs_destroy will clean up all the memory associated with
 *              a CSPARSE structure, including strings passed to
 *              cs_parse_string.  This does not clean up any memory
 *              allocated by your own CSOUTFUNC or the HDF data
 *              structure passed to cs_init.  It is safe to call this
 *              with a NULL pointer, and it will leave parse NULL as
 *              well (ie, it can be called more than once on the same
 *              var)
 * Input: parse - a pointer to a parse structure.
 * Output: parse - will be NULL
 * Return: None
 */
void cs_destroy (CSPARSE **parse);

#endif /* _CSHDF_H_ */
