/*
 * Copyright (C) 2000  Neotonic
 *
 * All Rights Reserved.
 */

#ifndef __NEO_ERR_H_
#define __NEO_ERR_H_ 1

__BEGIN_DECLS

#define STATUS_OK ((NEOERR *)0)
#define INTERNAL_ERR ((NEOERR *)1)

/* NEOERR flags */
#define NE_IN_USE (1<<0)

/* Sections */
#define SYSTEM_ERROR   (0x50)

/* Types */
extern int NERR_PASS;
extern int NERR_ASSERT;
extern int NERR_NOT_FOUND;
extern int NERR_DUPLICATE;
extern int NERR_NOMEM;
extern int NERR_PARSE;
extern int NERR_OUTOFRANGE;
extern int NERR_SYSTEM;
extern int NERR_IO;
extern int NERR_LOCK;

typedef struct _neo_err 
{
  int error;
  int err_stack;
  int flags;
  char desc[256];
  char *file;
  char *func;
  int lineno;
  /* internal use only */
  struct _neo_err *next;
} NEOERR;


/*
 * function: nerr_raise
 * description: Use this method to create an error "exception" for
 *              return up the call chain
 * arguments: using the macro, the function name, file, and lineno are
 *            automagically recorded for you.  You just provide the
 *            error (from those listed above) and the printf-style 
 *            reason.  THIS IS A PRINTF STYLE FUNCTION, DO NOT PASS
 *            UNKNOWN STRING DATA AS THE FORMAT STRING.
 * returns: a pointer to a NEOERR, or INTERNAL_ERR if allocation of
 *          NEOERR fails
 */
#define nerr_raise(e,f,a...) \
   nerr_raisef(__PRETTY_FUNCTION__,__FILE__,__LINE__,e,f,##a)

NEOERR *nerr_raisef (char *func, char *file, int lineno, int error, 
                    char *fmt, ...);

/* function: nerr_pass
 * description: this function is used to pass an error up a level in the
 *              call chain (ie, if the error isn't handled at the
 *              current level).  This allows us to track the traceback
 *              of the error.
 * arguments: with the macro, the function name, file and lineno are
 *            automagically recorded.  Just pass the error.
 * returns: a pointer to an error
 */
#define nerr_pass(e) \
   nerr_passf(__PRETTY_FUNCTION__,__FILE__,__LINE__,e)
NEOERR *nerr_passf (char *func, char *file, int lineno, NEOERR *err);

/* function: nerr_pass_ctx
 * description: this function is used to pass an error up a level in the
 *              call chain (ie, if the error isn't handled at the
 *              current level).  This allows us to track the traceback
 *              of the error.
 *              This version includes context information about lower
 *              errors
 * arguments: with the macro, the function name, file and lineno are
 *            automagically recorded.  Just pass the error and 
 *            a printf format string giving more information about where
 *            the error is occuring.
 * returns: a pointer to an error
 */
#define nerr_pass_ctx(e,f,a...) \
   nerr_pass_ctxf(__PRETTY_FUNCTION__,__FILE__,__LINE__,e,f,##a)
NEOERR *nerr_pass_ctxf (char *func, char *file, int lineno, NEOERR *err, 
                       char *fmt, ...);

/* function: nerr_log_error
 * description: currently, this prints out the error to stderr, and
 *             free's the error chain
 */
void nerr_log_error (NEOERR *err);

#include "neo_str.h"
void nerr_error_str (NEOERR *err, STRING *str);

/* function: nerr_ignore
 * description: you should only call this if you actually handle the
 *              error (should I rename it?).  Free's the error chain.
 */
void nerr_ignore (NEOERR *err);

NEOERR *nerr_register (int *err, char *name);

NEOERR *nerr_init (void);

int nerr_handle (NEOERR **err, int type);

__END_DECLS

#endif /* __NEO_ERR_H_ */
