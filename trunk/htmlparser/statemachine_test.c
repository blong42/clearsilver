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

enum state_list {
    A, B, C, D, E, F, ERROR
};

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

  statemachine_parse(sm, "001", 3);
  ASSERT(sm->current_state == B);

  res = statemachine_parse(sm, "3", 1);
  ASSERT(res == STATEMACHINE_ERROR);

  statemachine_delete(sm);
  return 0;
}

int main(int argc, char **argv)
{
  test_simple();
  test_error();
  printf("DONE.\n");
  return 0;
}
