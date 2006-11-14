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
 * Tests for ulist library
 */

#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/ulist.h"

NEOERR *TestIntegerStorage() {
  NEOERR *err;
  ULIST *arr;
  int x;

  // Simple append test
  err = uListInit(&arr, 100, 0);
  if (err) return nerr_pass(err);
  for (x = 0; x < 1000; x++) {
    err = uListAppend(arr, (void *)x);
    if (err) return nerr_pass(err);
  }

  for (x = 0; x < 1000; x++) {
    int y;

    err = uListGet(arr, x, (void *)&y);
    if (err) return nerr_pass(err);
    if (x != y) {
      return nerr_raise(NERR_ASSERT, 
          "Value returned didn't match value stored: Got %d, Expected %d", 
          y, x);
    }
  }

  return STATUS_OK;
}


int main(int argc, char *argv[]) {
  NEOERR *err;

  nerr_init();

  err = TestIntegerStorage();
  if (err) {
    nerr_log_error(err);
    printf("FAIL\n");
    return -1;
  }
  printf("PASS\n");
  return 0;
}
