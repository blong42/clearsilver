// Author: falmeida@google.com (Filipe Almeida)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//#define DEBUG

#include "statemachine.h"
#include "jsparser.h"
#include "jsparser_states_internal.h"

#define state_external(x) states_external[(int)(x)]

/* Initializes a new jsparser instance.
 *
 * Returns a pointer to the new instance or NULL if the initialization
 * fails.
 *
 * Initialization failure is fatal, and if this function fails it may not
 * deallocate all previsouly allocated memory.
 */

jsparser_ctx *jsparser_new()
{
    statemachine_ctx *sm;
    statemachine_definition *def;
    state **st;
    jsparser_ctx *js;

    def = statemachine_definition_new(NUM_STATES);
    if(!def)
      return NULL;

    sm = statemachine_new(def);
    if(!sm)
      return NULL;

    js = calloc(1, sizeof(jsparser_ctx));
    if(!js)
      return NULL;

    js->statemachine = sm;
    sm->user = js;
    st = def->transition_table;

    statetable_populate(st, state_transitions);

    return js;
}

void jsparser_reset(jsparser_ctx *ctx)
{
  assert(ctx);
  ctx->statemachine->current_state = 0;
}


state jsparser_state(jsparser_ctx *ctx)
{
  return state_external(ctx->statemachine->current_state);
}

state jsparser_parse(jsparser_ctx *ctx, const char *str, int size)
{
    state internal_state;
    internal_state = statemachine_parse(ctx->statemachine, str, size);
    return state_external(internal_state);
}

const char *jsparser_state_str(jsparser_ctx *ctx) {
    return
        states_external_name[(int)(state_external(ctx->statemachine->current_state))];
}

void jsparser_delete(jsparser_ctx *ctx)
{
    assert(ctx);
    statemachine_delete(ctx->statemachine);
    free(ctx);
}
