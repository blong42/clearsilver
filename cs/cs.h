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
 * CS Syntax (pseudo BNF, pseudo accurate)
 * ------------------------------------------------------------------
 * CS          := (ANYTHING COMMAND)*
 * CS_OPEN     := <?cs
 * CS_CLOSE    := ?>
 * COMMAND     := (CMD_IF | CMD_VAR | CMD_EVAR | CMD_INCLUDE | CMD_EACH
 *                 | CMD_DEF | CMD_CALL | CMD_SET | CMD_LOOP )
 * CMD_IF      := CS_OPEN IF CS_CLOSE CS CMD_ENDIF
 * CMD_ENDIF   := CS_OPEN ENDIF CS_CLOSE
 * CMD_INCLUDE := CS_OPEN INCLUDE CS_CLOSE
 * CMD_DEF     := CS_OPEN DEF CS_CLOSE
 * CMD_CALL    := CS_OPEN CALL CS_CLOSE
 * CMD_SET     := CS_OPEN SET CS_CLOSE
 * CMD_LOOP    := CS_OPEN LOOP CS_CLOSE
 * LOOP        := loop:VAR = EXPR, EXPR, EXPR
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

#ifndef __CSHDF_H_
#define __CSHDF_H_ 1

#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/ulist.h"
#include "util/neo_hdf.h"
#include "util/neo_str.h"

__BEGIN_DECLS

typedef enum
{
  /* Unary operators */
  CS_OP_NONE = (1<<0),
  CS_OP_EXISTS = (1<<1),
  CS_OP_NOT = (1<<2),
  CS_OP_NUM = (1<<3),

  /* Binary Operators */
  CS_OP_EQUAL = (1<<4),
  CS_OP_NEQUAL = (1<<5),
  CS_OP_LT = (1<<6),
  CS_OP_LTE = (1<<7),
  CS_OP_GT = (1<<8),
  CS_OP_GTE = (1<<9),
  CS_OP_AND = (1<<10),
  CS_OP_OR = (1<<11),
  CS_OP_ADD = (1<<12),
  CS_OP_SUB = (1<<13),
  CS_OP_MULT = (1<<14),
  CS_OP_DIV = (1<<15),
  CS_OP_MOD = (1<<16),

  /* Associative Operators */
  CS_OP_LPAREN = (1<<17),
  CS_OP_RPAREN = (1<<18),
  CS_OP_LBRACKET = (1<<19),
  CS_OP_RBRACKET = (1<<20),

  CS_OP_DOT = (1<<21),
  CS_OP_COMMA = (1<<22),

  /* Types */
  CS_TYPE_STRING = (1<<25),
  CS_TYPE_NUM = (1<<26),
  CS_TYPE_VAR = (1<<27),
  CS_TYPE_VAR_NUM = (1<<28),

  /* Not real types... */
  CS_TYPE_MACRO = (1<<29),
  CS_TYPE_FUNCTION = (1<<30)
} CSTOKEN_TYPE;

#define CS_OPS_UNARY (CS_OP_EXISTS | CS_OP_NOT | CS_OP_NUM | CS_OP_LPAREN)
#define CS_TYPES (CS_TYPE_STRING | CS_TYPE_NUM | CS_TYPE_VAR | CS_TYPE_VAR_NUM)
#define CS_OPS_LVALUE (CS_OP_DOT | CS_OP_LBRACKET | CS_TYPES)
#define CS_TYPES_VAR (CS_TYPE_VAR | CS_TYPE_VAR_NUM)
#define CS_TYPES_CONST (CS_TYPE_STRING | CS_TYPE_NUM)
#define CS_ASSOC (CS_OP_RPAREN | CS_OP_RBRACKET)

typedef struct _parse CSPARSE;
typedef struct _funct CS_FUNCTION;
typedef struct _escape_context CS_ECONTEXT;
typedef struct _position CS_POSITION;
typedef struct _error CS_ERROR;

typedef struct _arg
{
  CSTOKEN_TYPE op_type;
  char *argexpr;
  char *s;
  long int n;
  int alloc;
  struct _funct *function;
  struct _macro *macro;
  struct _arg *expr1;
  struct _arg *expr2;
  struct _arg *next;
} CSARG;

#define CSF_REQUIRED (1<<0)

typedef struct _tree 
{
  int node_num;
  int cmd;
  int flags;
  NEOS_ESCAPE escape;
  CSARG arg1;
  CSARG arg2;
  CSARG *vargs;

  char *fname;
  int linenum;
  int colnum;

  struct _tree *case_0;
  struct _tree *case_1;
  struct _tree *next;
} CSTREE;

typedef struct _local_map
{
  CSTOKEN_TYPE type;
  char *name;
  int map_alloc;
  /* These three (s,n,h) used to be a union, but now we sometimes allocate
   * a buffer in s with the "string" value of n, so its separate */
  char *s;
  long int n;
  HDF *h;
  int first;  /* This local is the "first" item in an each/loop */
  int last;   /* This local is the "last" item in an loop, each is calculated
               explicitly based on hdf_obj_next() in _builtin_last() */
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


/* CSOUTFUNC is a callback function for where cs_render will render the 
 * template to.
 * Technically, the char * for this func should be const char *, but that
 * would break existing code. */
typedef NEOERR* (*CSOUTFUNC)(void *, char *);

/* CSFUNCTION is a callback function used for handling a function made
 * available inside the template.  Used by cs_register_function.  Exposed
 * here as part of the experimental extension framework, this may change
 * in future versions. */
typedef NEOERR* (*CSFUNCTION)(CSPARSE *parse, CS_FUNCTION *csf, CSARG *args,
                              CSARG *result);

/* CSSTRFUNC is a callback function for the more limited "string filter"
 * function handling.  String filters only take a string and return a 
 * string and have no "data" that is passed back, attempting to make them
 * "safe" from extensions that might try to do silly things like SQL queries.
 * */
typedef NEOERR* (*CSSTRFUNC)(const char *str, char **ret);

/* CSFILELOAD is a callback function to intercept file load requests and
 * provide templates via another mechanism.  This way you can load templates
 * that you compiled-into your binary, from in-memory caches, or from a
 * zip file, etc.  The HDF is provided so you can choose to use the 
 * hdf_search_path function to find the file.  contents should return
 * a full malloc copy of the contents of the file, which the parser will modify
 * and own and free.  Use cs_register_fileload to set this function for
 * your CSPARSE context. */
typedef NEOERR* (*CSFILELOAD)(void *ctx, HDF *hdf, const char *filename,
                              char **contents);

struct _funct
{
  char *name;
  int name_len;
  int n_args;
  NEOS_ESCAPE escape; /* States escaping exemptions. default: NONE. */

  CSFUNCTION function;
  CSSTRFUNC str_func;

  struct _funct *next;
};

/* This structure maintains the necessary running state to manage
 * automatic escaping in tandem with the 'escape' directive. It is passed
 * around inside _parse.
 */
struct _escape_context
{
  NEOS_ESCAPE global_ctx; /* Contains global default escaping mode:
			     none,html,js,url */
  NEOS_ESCAPE current;    /* Used to pass around parse and evaluation specific
                             data from subfunctions upward. */
  NEOS_ESCAPE next_stack; /* This is a big fat workaround. Since STACK_ENTRYs
                             are only added to the stack after the
                             command[].parse_handler() is called for the call
                             it is being setup for, this is used to pass state
                             forward.  E.g. This is used for 'def' to set UNDEF
                             escaping state to delay escaping status to
                             evaluation time. */
  NEOS_ESCAPE when_undef; /* Contains the escaping context to be used when a
                             UNDEF is being replaced at evaluation time.  E.g.
                             this is set in call_eval to force the macro code
                             to get call's parsing context at eval time. */
};

/* This structure is used to track current location within the CS file being
 * parsed. This information is used to find the filename and line number for
 * each node.
 */
struct _position {
  int line;        /* Line number for current position */
  int col;         /* Column number for current position */
  int cur_offset;  /* The current position - commence reading from here */
};

struct _error {
  NEOERR *err;
  struct _error *next;
};
  
struct _parse
{
  const char *context;   /* A string identifying where the parser is parsing */
  int in_file;           /* Indicates if current context is a file */
  int offset;

  int audit_mode;        /* If in audit_mode, gather some extra information */
  CS_POSITION pos;       /* Container for current position in CS file */
  CS_ERROR *err_list;    /* List of non-fatal errors encountered */
  
  char *context_string;
  CS_ECONTEXT escaping; /* Context container for escape data */

  char *tag;		/* Usually cs, but can be set via HDF Config.TagStart */
  int taglen;

  ULIST *stack;
  ULIST *alloc;         /* list of strings owned by CSPARSE and free'd when
                           its destroyed */
  CSTREE *tree;
  CSTREE *current;
  CSTREE **next;

  HDF *hdf;

  struct _parse *parent;  /* set on internally created parse instances to point
                             at the parent.  This can be used for hierarchical
                             scope in the future. */

  CS_LOCAL_MAP *locals;
  CS_MACRO *macros;
  CS_FUNCTION *functions;

  /* Output */
  void *output_ctx;
  CSOUTFUNC output_cb;

  void *fileload_ctx;
  CSFILELOAD fileload;

  /* Global hdf struct */
  /* smarti:  Added for support for global hdf under local hdf */
  HDF *global_hdf;
};

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
NEOERR *cs_parse_file (CSPARSE *parse, const char *path);

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

/*
 * Function: cs_register_fileload - register a fileload function
 * Description: cs_register_fileload registers a fileload function that 
 *              overrides the built-in function.  The built-in function
 *              uses hdf_search_path and ne_file_load (based on stat/open/read)
 *              to find and load the file on every template render.
 *              You can override this function if you wish to provide
 *              other template search functions, or load the template
 *              from an in-memory cache, etc.
 *              This fileload function will be used by cs_parse, including
 *              any include: commands in the template.
 * Input: parse - a pointer to an initialized CSPARSE structure
 *        ctx - pointer that is passed to the CSFILELOAD function when called
 *        fileload - a CSFILELOAD function
 * Output: None
 * Return: None
 *
 */

void cs_register_fileload(CSPARSE *parse, void *ctx, CSFILELOAD fileload);

/*
 * Function: cs_register_strfunc - register a string handling function
 * Description: cs_register_strfunc will register a string function that
 *              can be called during CS render.  This not-callback is 
 *              designed to allow for string formating/escaping
 *              functions that are not built-in to CS (since CS is not
 *              HTML specific, for instance, but it is very useful to
 *              have CS have functions for javascript/html/url
 *              escaping).  Note that we explicitly don't provide any
 *              associated data or anything to attempt to keep you from
 *              using this as a generic callback...
 *              The format of a CSSTRFUNC is:
 *                 NEOERR * str_func(char *in, char **out);
 *              This function should not modify the input string, and 
 *              should allocate the output string with a libc function.
 *              (as we will call free on it)
 * Input: parse - a pointer to a CSPARSE structure initialized with cs_init()
 *        funcname - the name for the CS function call
 *                   Note that registering a duplicate funcname will
 *                   raise a NERR_DUPLICATE error
 *        str_func - a CSSTRFUNC not-callback
 * Return: NERR_NOMEM - failure to allocate any memory for data structures
 *         NERR_DUPLICATE - funcname already registered
 *          
 */
NEOERR *cs_register_strfunc(CSPARSE *parse, char *funcname, CSSTRFUNC str_func);

/*
 * Function: cs_register_esc_strfunc - cs_register_strfunc with escaping context
 * Description: cs_register_esc_strfunc functions exactly as cs_register_strfunc
 *              except that it changes the evaluation escaping context to disable
 *              default escaping.
 * Input: parse - a pointer to a CSPARSE structure initialized with cs_init()
 *        funcname - the name for the CS function call
 *                   Note that registering a duplicate funcname will
 *                   raise a NERR_DUPLICATE error
 *        str_func - a CSSTRFUNC not-callback
 * Return: NERR_NOMEM - failure to allocate any memory for data structures
 *         NERR_DUPLICATE - funcname already registered
 *
 */
NEOERR *cs_register_esc_strfunc(CSPARSE *parse, char *funcname,
                                CSSTRFUNC str_func);

/* Testing functions for future function api.  This api may change in the
 * future. */
NEOERR *cs_arg_parse(CSPARSE *parse, CSARG *args, const char *fmt, ...);
NEOERR *cs_arg_parsev(CSPARSE *parse, CSARG *args, const char *fmt, va_list ap);
NEOERR *cs_register_function(CSPARSE *parse, const char *funcname,
                                  int n_args, CSFUNCTION function);

__END_DECLS

#endif /* __CSHDF_H_ */
