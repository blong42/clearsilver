
/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the FSF's
 * Library Gnu Public License (LGPL).
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef _ULIST_H_
#define _ULIST_H_ 1

#include "neo_err.h"

typedef struct _ulist
{
  int flags;
  void **items;
  int num;
  int max;
} ULIST;

#define ULIST_INTEGER (1<<0)
#define ULIST_FREE (1<<1)
#define ULIST_COPY (1<<2)

NEOERR * uListInit(ULIST **ul, int size, int flags);
NEOERR * uListvInit(ULIST **ul, ...);
int uListLength (ULIST *ul);
NEOERR * uListAppend (ULIST *ul, void *data);
NEOERR * uListPop (ULIST *ul, void **data);
NEOERR * uListInsert (ULIST *ul, int x, void *data);
NEOERR * uListDelete (ULIST *ul, int x, void **data);
NEOERR * uListGet (ULIST *ul, int x, void **data);
NEOERR * uListSet (ULIST *ul, int x, void *data);
NEOERR * uListReverse (ULIST *ul);
NEOERR * uListSort (ULIST *ul, int (*compareFunc)(const void*, const void*));
NEOERR * uListDestroy (ULIST **ul, int flags);
NEOERR * uListDestroyFunc (ULIST **ul, void (*destroyFunc)(void *));

#endif /* _ULIST_H_ */
