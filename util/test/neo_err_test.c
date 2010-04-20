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

#include "cs_config.h"
#include <unistd.h>
#include <string.h>
#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/neo_str.h"
#include "util/test/test_macros.h"


void test_no_nerr_init() {
  NEOERR *err;
  STRING buf;

  string_init(&buf);
  err = nerr_raise(NERR_ASSERT, "This is an error!");
  CHECK((err != STATUS_OK));
  nerr_error_string(err, &buf);
  nerr_ignore(&err);
  CHECK_STREQ(buf.buf, "Unknown Error: This is an error!");
}

int main(int argc, char *argv[]) {
  // Run this test first, before any calls to nerr_init
  test_no_nerr_init();

  ne_warn("PASS");
  return 0;
}
