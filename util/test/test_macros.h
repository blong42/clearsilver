/*
 * Copyright 2009 Brandon Long
 * All Rights Reserved.
 *
 * ClearSilver Templating System
 *
 * This code is made available under the terms of the ClearSilver License.
 * http://www.clearsilver.net/license.hdf
 *
 */

#ifndef __UTIL_TEST_TEST_MACROS_H_
#define __UTIL_TEST_TEST_MACROS_H_ 1

#define DIE_NOT_OK(err) \
  if (err != STATUS_OK) { \
    nerr_log_error(err); \
    exit(-1); \
  }

#define CHECK_STREQ(a, b) \
  if (strcmp(a, b)) { \
    ne_warn("FAIL: '%s' != '%s'", a, b); \
    exit(-1); \
  }

#define CHECK(a) \
  if (!a) { \
    ne_warn("FAIL: %s is not true", #a); \
  }

#endif  // __UTIL_TEST_TEST_MACROS_H_
