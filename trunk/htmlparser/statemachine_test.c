#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "change_defs.h"
#include "statemachine.h"

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


#define NUM_STATES 10

/* To simply the tests */
#define statemachine_parse_str(a,b) statemachine_parse(a, b, strlen(b));

enum state_list {
    A, B, C, D, E, F, ERROR
};

/* Simple state machine test. */
int test_simple()
{
  statemachine_definition *def;
  statemachine_ctx *sm;
  def = statemachine_definition_new(NUM_STATES);
  sm = statemachine_new(def);

  struct statetable_transitions_s transitions[] = {
    { "[:default:]", A, A },
    { "1", A, B },
    { "[:default:]", B, B },
    { "1", B, C },
    { "2", B, A },
    { "[:default:]", C, C },
    { "1", C, D },
    { "2", C, B },
    { "[:default:]", D, D },
    { "2", D, C },
    { NULL, ERROR, ERROR }
  };

  statemachine_definition_populate(def, transitions);
  ASSERT(sm->current_state == A);

  statemachine_parse(sm, "001", 3);
  ASSERT(sm->current_state == B);

  statemachine_parse(sm, "001", 3);
  ASSERT(sm->current_state == C);

  statemachine_parse(sm, "2", 1);
  ASSERT(sm->current_state == B);

  statemachine_parse(sm, "11", 2);
  ASSERT(sm->current_state == D);

  statemachine_delete(sm);
  return 0;
}

/* Tests error handling logic when we try to follow non existent transitions. */
int test_error()
{
  statemachine_definition *def;
  statemachine_ctx *sm;
  int res;
  def = statemachine_definition_new(NUM_STATES);
  sm = statemachine_new(def);

  struct statetable_transitions_s transitions[] = {
    { "[:default:]", A, A },
    { "1", A, B },
    { "1", B, C },
    { "2", B, A },
    { NULL, ERROR, ERROR }
  };

  statemachine_definition_populate(def, transitions);
  ASSERT(sm->current_state == A);

  res = statemachine_parse_str(sm, "001");
  ASSERT(sm->current_state == B);
  ASSERT(sm->current_state == res);

  res = statemachine_parse(sm, "3", 1);
  ASSERT(res == STATEMACHINE_ERROR);


  statemachine_delete(sm);
  return 0;
}

/* Tests htmlparser_start_record() and htmlparser_end_record() logic. */
int test_record()
{
  statemachine_definition *def;
  statemachine_ctx *sm;
  const char *actual;
  char expected[STATEMACHINE_RECORD_BUFFER_SIZE];
  int res;
  int counter;
  def = statemachine_definition_new(NUM_STATES);
  sm = statemachine_new(def);

  struct statetable_transitions_s transitions[] = {
    { "[:default:]", A, A },
    { "1", A, B },
    { "[:default:]", B, B },
    { "2", B, A },
    { NULL, ERROR, ERROR }
  };

  statemachine_definition_populate(def, transitions);
  ASSERT(sm->current_state == A);

  res = statemachine_parse_str(sm, "001");
  ASSERT(sm->current_state == B);
  ASSERT(sm->current_state == res);

  statemachine_start_record(sm);
  statemachine_parse_str(sm, "121212");
  ASSERT_STREQ("121212", statemachine_stop_record(sm));

  statemachine_parse_str(sm, "not recorded");

  statemachine_start_record(sm);
  statemachine_parse_str(sm, "121212000");
  ASSERT_STREQ("121212000", statemachine_stop_record(sm));

  statemachine_start_record(sm);
  statemachine_parse_str(sm, "1234567890");
  ASSERT_STREQ("1234567890", statemachine_record_buffer(sm));

  statemachine_parse_str(sm, "test");
  ASSERT_STREQ("1234567890test", statemachine_stop_record(sm));

  statemachine_start_record(sm);

  /* Record 1000 chars + strlen("beginning-") */
  statemachine_parse_str(sm, "beginning-");
  for (counter = 0; counter < 100; counter++) {
    statemachine_parse_str(sm, "1234567890");
  }

  /* Make sure we preserved the start of the buffer. */
  ASSERT_STRSTR(statemachine_record_buffer(sm), "beginning-");

  /* And make sure the size is what we expect. */
  ASSERT(STATEMACHINE_RECORD_BUFFER_SIZE - 1 ==
         strlen(statemachine_stop_record(sm)));

  statemachine_start_record(sm);
  for (counter = 0; counter < 100; counter++) {
    statemachine_parse_str(sm, "0123456789ABCDEF");
  }

  expected[0] = '\0';
  /* Fill the buffer with a pattern 255 chars long (16 * 15 + 15). */
  for (counter = 0; counter < 15; counter++) {
    strcat(expected, "0123456789ABCDEF");
  }
  strcat(expected, "0123456789ABCDE");
  actual = statemachine_stop_record(sm);
  ASSERT_STREQ(expected, actual);

  statemachine_delete(sm);
  return 0;
}

/* Test with characters outside of the ascii range */
int test_no_ascii()
{
  statemachine_definition *def;
  statemachine_ctx *sm;
  def = statemachine_definition_new(NUM_STATES);
  sm = statemachine_new(def);

  struct statetable_transitions_s transitions[] = {
    { "[:default:]", A, A },
    { "\xf1", A, B },
    { "[:default:]", B, B },
    { "\xf1", B, C },
    { "\xf2", B, A },
    { "[:default:]", C, C },
    { "\xf1", C, D },
    { "\xf2", C, B },
    { "[:default:]", D, D },
    { "\xf2", D, C },
    { NULL, ERROR, ERROR }
  };

  statemachine_definition_populate(def, transitions);
  ASSERT(sm->current_state == A);

  statemachine_parse(sm, "\xf0\xf0\xf1", 3);
  ASSERT(sm->current_state == B);

  statemachine_parse(sm, "\xf0\xf0\xf1", 3);
  ASSERT(sm->current_state == C);

  statemachine_parse(sm, "\xf2", 1);
  ASSERT(sm->current_state == B);

  statemachine_parse(sm, "\xf1\xf1", 2);
  ASSERT(sm->current_state == D);

  statemachine_delete(sm);
  return 0;

}


int main(int argc, char **argv)
{
  test_simple();
  test_error();
  test_record();
  test_no_ascii();
  printf("DONE.\n");
  return 0;
}
