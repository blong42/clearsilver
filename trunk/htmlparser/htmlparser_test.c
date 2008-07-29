/* Copyright 2008 Google Inc. All Rights Reserved.
 * Author: falmeida@google.com (Filipe Almeida)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "security/streamhtmlparser/htmlparser.h"

/* Taken from google templates */

#define ASSERT(cond)  do {                                      \
  if (!(cond)) {                                                \
    printf("%s: %d: ASSERT FAILED: %s\n", __FILE__, __LINE__,   \
           #cond);                                              \
    assert(cond);                                               \
    exit(1);                                                    \
  }                                                             \
} while (0)

#define ASSERT_STREQ(a, b)  do {                                          \
  if (strcmp((a), (b))) {                                                 \
    printf("%s: %d: ASSERT FAILED: '%s' != '%s'\n", __FILE__, __LINE__,   \
           (a), (b));                                                     \
    assert(!strcmp((a), (b)));                                            \
    exit(1);                                                              \
  }                                                                       \
} while (0)

#define ASSERT_STRSTR(text, substr)  do {                       \
  if (!strstr((text), (substr))) {                              \
    printf("%s: %d: ASSERT FAILED: '%s' not in '%s'\n",         \
           __FILE__, __LINE__, (substr), (text));               \
    assert(strstr((text), (substr)));                           \
    exit(1);                                                    \
  }                                                             \
} while (0)


/* Process a string using entityfilter_process(). */
void entityfilter_process_str(entityfilter_ctx *filter, const char *in,
                              char *out)
{
  out[0] = '\0';
  while (*in) {
    strcat(out, entityfilter_process(filter, *in));
    ++in;
  }
}

/* Verify that the code still builds with a C compiler. */
void test_c_build()
{
  htmlparser_ctx *html;

#ifdef __cplusplus
  printf("C build test compiled in c++ mode.");
  exit(1);
#endif /* __cplusplus */

  html = htmlparser_new();
  htmlparser_delete(html);
  printf("DONE.\n");
}

/* Entity filter tests. */
void test_entityfilter()
{
  entityfilter_ctx *filter = entityfilter_new();
  char buffer[256];

  entityfilter_process_str(filter, "test", buffer);
  ASSERT_STREQ("test", buffer);

  entityfilter_process_str(filter, "testtest", buffer);
  ASSERT_STREQ("testtest", buffer);

  entityfilter_process_str(filter, "test&apos;test", buffer);
  ASSERT_STREQ("test'test", buffer);

  entityfilter_process_str(filter, "test&#39;test", buffer);
  ASSERT_STREQ("test'test", buffer);

  entityfilter_process_str(filter, "test&#x27;test", buffer);
  ASSERT_STREQ("test'test", buffer);

  entityfilter_process_str(filter, "A&#65;&#x41;A", buffer);
  ASSERT_STREQ("AAAA", buffer);

  entityfilter_process_str(filter, "A&#65 &#x41 A", buffer);
  ASSERT_STREQ("AAAA", buffer);

  entityfilter_process_str(filter, "test&invalid;test", buffer);
  ASSERT_STREQ("test&invalid;test", buffer);

  entityfilter_process_str(filter, "test&invalid;01234567890123456789", buffer);
  ASSERT_STREQ("test&invalid;01234567890123456789", buffer);

  entityfilter_process_str(filter, "test&incomplete01234567890123456789", buffer);
  ASSERT_STREQ("test&incomplete01234567890123456789", buffer);

  entityfilter_process_str(filter, "test&012345;big", buffer);
  ASSERT_STREQ("test&012345;big", buffer);

  entityfilter_process_str(filter, "test&0123456;big", buffer);
  ASSERT_STREQ("test&0123456;big", buffer);

  entityfilter_process_str(filter, "test&01234567;big", buffer);
  ASSERT_STREQ("test&01234567;big", buffer);

  entityfilter_process_str(filter, "test&012345678;big", buffer);
  ASSERT_STREQ("test&012345678;big", buffer);

  entityfilter_process_str(filter, "test&0123456789;big", buffer);
  ASSERT_STREQ("test&0123456789;big", buffer);

  entityfilter_process_str(filter, "test&01234567890;big", buffer);
  ASSERT_STREQ("test&01234567890;big", buffer);

  entityfilter_process_str(filter, "test&012345678901;big", buffer);
  ASSERT_STREQ("test&012345678901;big", buffer);

  entityfilter_process_str(filter, "test& & & & & & & & & & &", buffer);
  ASSERT_STREQ("test& & & & & & & & & & ", buffer);
}

int main(int argc, char **argv)
{
  test_c_build();
  test_entityfilter();
  printf("DONE.\n");
  return 0;
}
