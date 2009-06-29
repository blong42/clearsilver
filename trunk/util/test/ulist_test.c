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
#include "util/neo_rand.h"
#include "util/ulist.h"

#include <string.h>

int integerCompare(const void *a, const void *b) {
  int ax, bx;
  if (a == NULL) return -1;
  if (b == NULL) return 1;
  ax = *(int *)a;
  bx = *(int *)b;

  return ax - bx;
}

NEOERR *TestIntegerStorage() {
  NEOERR *err;
  ULIST *arr;
  int x;

  ne_warn("TestIntegerStorage");

  // Simple append test
  err = uListInit(&arr, 100, 0);
  if (err) return nerr_pass(err);
  for (x = 0; x < 1000; x++) {
    err = uListAppend(arr, (void *)x);
    if (err) return nerr_pass(err);
  }

  // Simple get test
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

  // Sort shouldn't do anything
  err = uListSort(arr, integerCompare);
  if (err) return nerr_pass(err);
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

  // Simple search test
  for (x = 0; x < 1000; x++) {
    int y = *(int *) uListSearch(arr, (void *)&x, integerCompare);
    if (x != y) {
      return nerr_raise(NERR_ASSERT,
          "Value returned didn't match value stored: Got %d, Expected %d",
          y, x);
    }
  }

  // Simple in test
  for (x = 0; x < 1000; x++) {
    int y = *(int *) uListIn(arr, (void *)&x, integerCompare);
    if (x != y) {
      return nerr_raise(NERR_ASSERT,
          "Value returned didn't match value stored: Got %d, Expected %d",
          y, x);
    }
  }

  err = uListDestroy(&arr, 0);
  return nerr_pass(err);
}

int stringCompare(const void *a, const void *b) {
  const char *sa, *sb;
  if (a == NULL) return -1;
  if (b == NULL) return 1;
  sa = *(char **)a;
  sb = *(char **)b;
  return strcmp(sa, sb);
}

NEOERR *TestStringStorage() {
  NEOERR *err;
  ULIST *arr;
  char word[4096];
  int x, y;
  char *a, *b;

  ne_warn("TestStringStorage");

  // Simple append test
  err = uListInit(&arr, 100, 0);
  if (err) return nerr_pass(err);
  for (x = 0; x < 1000; x++) {
    char *word_dup;
    neo_rand_word(word, sizeof(word));
    word_dup = strdup(word);
    if (word_dup == NULL)
      return nerr_raise(NERR_NOMEM, "Unable to dup word: %s", word);
    err = uListAppend(arr, word_dup);
    if (err) return nerr_pass(err);
  }

#ifdef DEBUG_DUMP
  for (x = 0; x < 1000; x++) {
    err = uListGet(arr, x, (void *)&a);
    if (err) return nerr_pass(err);
    ne_warn("%d = %s", x, a);
  }
#endif

  // Sort 'em
  err = uListSort(arr, stringCompare);
  if (err) return nerr_pass(err);

#ifdef DEBUG_DUMP
  for (x = 0; x < 1000; x++) {
    err = uListGet(arr, x, (void *)&a);
    if (err) return nerr_pass(err);
    ne_warn("%d = %s", x, a);
  }
#endif

  err = uListGet(arr, 0, (void *)&a);
  if (err) return nerr_pass(err);

  for (x = 1; x < 1000; x++) {
    err = uListGet(arr, x, (void *)&b);
    if (err) return nerr_pass(err);
    if (strcmp(a, b) > 0) {
      return nerr_raise(NERR_ASSERT,
          "Expect word %s to come after %s after sort", b, a);
    }
  }

  // Simple search test
  for (x = 0; x < 1000; x++) {
    err = uListGet(arr, x, (void *)&a);
    if (err) return nerr_pass(err);
    b = uListSearch(arr, &a, stringCompare);
    if (b == NULL) {
      return nerr_raise(NERR_ASSERT, "Unable to find %s", a);
    }
    if (!strcmp(a, b)) {
      return nerr_raise(NERR_ASSERT, "Expected to find %s, found %s", a, b);
    }
  }

  // Simple index test
  for (x = 0; x < 1000; x++) {
    err = uListGet(arr, x, (void *)&a);
    if (err) return nerr_pass(err);
    y = uListIndex(arr, &a, stringCompare);
    err = uListGet(arr, y, (void *)&b);
    if (x != y) {
      err = uListGet(arr, y, (void *)&b);
      if (err) return nerr_pass(err);
      if (strcmp(a, b)) {
        return nerr_raise(NERR_ASSERT, "Index returned didn't match index "
                          "expected: Got %d, Expected %d", y, x);
      }
    }
  }

  err = uListDestroy(&arr, 0);
  return nerr_pass(err);
}

int main(int argc, char *argv[]) {
  NEOERR *err;

  nerr_init();

  err = TestStringStorage();
  if (err) {
    nerr_log_error(err);
    printf("FAIL\n");
    return -1;
  }

  err = TestIntegerStorage();
  if (err) {
    nerr_log_error(err);
    printf("FAIL\n");
    return -1;
  }
  printf("PASS\n");
  return 0;
}
