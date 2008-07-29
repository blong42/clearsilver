/* Copyright 2008 Google Inc. All Rights Reserved.
 * Author: falmeida@google.com (Filipe Almeida)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "security/streamhtmlparser/jsparser.h"

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


/* Tests for jsparser_buffer_get(). */
void test_buffer_get()
{
  jsparser_ctx *js;
  js = jsparser_new();

  ASSERT(jsparser_buffer_get(js, -1) == '\0');
  ASSERT(jsparser_buffer_get(js, -2) == '\0');
  ASSERT(jsparser_buffer_get(js, -3) == '\0');

  jsparser_buffer_append_chr(js, 'a');
  ASSERT(jsparser_buffer_get(js, -1) == 'a');
  ASSERT(jsparser_buffer_get(js, -2) == '\0');


  jsparser_buffer_append_chr(js, 'b');
  ASSERT(jsparser_buffer_get(js, -1) == 'b');
  ASSERT(jsparser_buffer_get(js, -2) == 'a');
  ASSERT(jsparser_buffer_get(js, -3) == '\0');

  jsparser_buffer_append_str(js, "1234567890");
  ASSERT(jsparser_buffer_get(js, -1) == '0');
  ASSERT(jsparser_buffer_get(js, -2) == '9');
  ASSERT(jsparser_buffer_get(js, -3) == '8');

  jsparser_buffer_append_str(js, "ABCDEGHIJKLMN");
  ASSERT(jsparser_buffer_get(js, -1) == 'N');
  ASSERT(jsparser_buffer_get(js, -2) == 'M');
  ASSERT(jsparser_buffer_get(js, -3) == 'L');
  ASSERT(jsparser_buffer_get(js, -200) == '\0');

  jsparser_delete(js);
}

/* Tests for jsparser_buffer_set(). */
void test_buffer_set()
{
  jsparser_ctx *js;
  js = jsparser_new();

  ASSERT(jsparser_buffer_set(js, -1, 'a') == 0);
  ASSERT(jsparser_buffer_set(js, -2, 'b') == 0);
  ASSERT(jsparser_buffer_set(js, -3, 'c') == 0);

  jsparser_buffer_append_chr(js, 'a');
  ASSERT(jsparser_buffer_set(js, -1, 'b') != 0);
  ASSERT(jsparser_buffer_get(js, -1) == 'b');

  jsparser_delete(js);
}

/* Tests for jsparser_buffer_pop(). */
void test_buffer_pop()
{
  jsparser_ctx *js;
  js = jsparser_new();

  ASSERT(jsparser_buffer_pop(js) == '\0');

  jsparser_buffer_append_str(js, "012345");
  ASSERT(jsparser_buffer_pop(js) == '5');
  ASSERT(jsparser_buffer_pop(js) == '4');
  ASSERT(jsparser_buffer_pop(js) == '3');
  ASSERT(jsparser_buffer_pop(js) == '2');
  ASSERT(jsparser_buffer_pop(js) == '1');
  ASSERT(jsparser_buffer_pop(js) == '0');
  ASSERT(jsparser_buffer_pop(js) == '\0');

  jsparser_buffer_append_str(js, "ABCDEGHIJKLMN");
  jsparser_buffer_append_str(js, "ABCDEGHIJKLMN");
  jsparser_buffer_append_str(js, "012345");
  ASSERT(jsparser_buffer_pop(js) == '5');
  ASSERT(jsparser_buffer_pop(js) == '4');
  ASSERT(jsparser_buffer_pop(js) == '3');
  ASSERT(jsparser_buffer_pop(js) == '2');
  ASSERT(jsparser_buffer_pop(js) == '1');
  ASSERT(jsparser_buffer_pop(js) == '0');
  ASSERT(jsparser_buffer_pop(js) == 'N');

  jsparser_delete(js);
}

/* Tests for jsparser_buffer_last_identifier(). */
void test_buffer_last_identifier()
{
  jsparser_ctx *js;
  char buffer[256];
  js = jsparser_new();

  jsparser_buffer_append_str(js, "abc");
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("abc", buffer);

  jsparser_buffer_append_str(js, "abc");
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("abcabc", buffer);

  jsparser_buffer_append_str(js, " abc2");
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("abc2", buffer);

  jsparser_buffer_append_str(js, " abc3 ");
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("abc3", buffer);

  jsparser_buffer_append_str(js, " abc4  ");
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("abc4", buffer);

  jsparser_buffer_append_str(js, "  abc5  ");
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("abc5", buffer);

  jsparser_buffer_append_str(js, "test     testtesttest");
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("testtesttest", buffer);

  jsparser_buffer_append_str(js, "01234567890123456789");
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("34567890123456789", buffer);

  jsparser_delete(js);
}

/* Tests for jsparser_buffer_slice(). */
void test_buffer_slice()
{
  jsparser_ctx *js;
  char buffer[256];
  js = jsparser_new();

  jsparser_buffer_append_str(js, "test");
  jsparser_buffer_slice(js, buffer, -4, -1);
  ASSERT_STREQ("test", buffer);

  jsparser_buffer_slice(js, buffer, -10, -1);
  ASSERT_STREQ("test", buffer);

  jsparser_buffer_append_str(js, "     test2");
  jsparser_buffer_slice(js, buffer, -5, -1);
  ASSERT_STREQ("test2", buffer);

  jsparser_buffer_slice(js, buffer, -9, -1);
  ASSERT_STREQ("est test2", buffer);

  jsparser_buffer_slice(js, buffer, -10, -1);
  ASSERT_STREQ("test test2", buffer);

  jsparser_buffer_slice(js, buffer, -100, -1);
  ASSERT_STREQ("test test2", buffer);

  jsparser_buffer_append_str(js, " \n\r test3 \n\r ");
  jsparser_buffer_slice(js, buffer, -6, -1);
  ASSERT_STREQ("test3 ", buffer);

  jsparser_buffer_slice(js, buffer, -10, -1);
  ASSERT_STREQ("st2 test3 ", buffer);

  jsparser_buffer_slice(js, buffer, -12, -1);
  ASSERT_STREQ("test2 test3 ", buffer);

  jsparser_buffer_slice(js, buffer, -17, -1);
  ASSERT_STREQ("test test2 test3 ", buffer);

  jsparser_buffer_slice(js, buffer, -100, -1);
  ASSERT_STREQ("test test2 test3 ", buffer);

  jsparser_buffer_append_str(js, "0123456789");
  jsparser_buffer_append_str(js, "0123456789");
  jsparser_buffer_append_str(js, "0123456789");
  jsparser_buffer_append_str(js, "0123456789");
  jsparser_buffer_append_str(js, "0123456789");

  jsparser_buffer_slice(js, buffer, -10, -1);
  ASSERT_STREQ("0123456789", buffer);

  jsparser_buffer_append_str(js, "          ");
  jsparser_buffer_append_str(js, "          ");
  jsparser_buffer_append_str(js, "          ");
  jsparser_buffer_append_str(js, "0123456789");
  jsparser_buffer_slice(js, buffer, -11, -1);
  ASSERT_STREQ(" 0123456789", buffer);

  jsparser_buffer_slice(js, buffer, -13, -1);
  ASSERT_STREQ("89 0123456789", buffer);


  jsparser_delete(js);
}

/* Tests for combination of calls. */
void test_buffer_misc()
{
  jsparser_ctx *js;
  char buffer[256];
  js = jsparser_new();

  jsparser_buffer_append_str(js, "012345 test test");
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("test", buffer);

  ASSERT(jsparser_buffer_pop(js) == 't');
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("tes", buffer);

  jsparser_buffer_append_chr(js, 'X');
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("tesX", buffer);

  jsparser_buffer_set(js, -3, 'W');
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("tWsX", buffer);

  jsparser_buffer_append_chr(js, ' ');
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("tWsX", buffer);

  jsparser_buffer_append_chr(js, '\n');
  jsparser_buffer_last_identifier(js, buffer);
  ASSERT_STREQ("tWsX", buffer);

  jsparser_reset(js);

  ASSERT(jsparser_buffer_get(js, -1) == '\0');

  jsparser_buffer_append_str(js, "0123456789");
  jsparser_buffer_append_str(js, "0123456789");
  ASSERT(jsparser_buffer_pop(js) == '9');
  ASSERT(jsparser_buffer_pop(js) == '8');
  ASSERT(jsparser_buffer_pop(js) == '7');
  ASSERT(jsparser_buffer_pop(js) == '6');
  ASSERT(jsparser_buffer_pop(js) == '5');
  ASSERT(jsparser_buffer_pop(js) == '4');
  ASSERT(jsparser_buffer_pop(js) == '3');

  jsparser_delete(js);
}

int main(int argc, char **argv)
{
  test_buffer_get();
  test_buffer_set();
  test_buffer_pop();
  test_buffer_last_identifier();
  test_buffer_slice();
  test_buffer_misc();
  printf("DONE.\n");
  return 0;
}
