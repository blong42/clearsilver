// Author: falmeida@google.com (Filipe Almeida)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//#define DEBUG

#include "statemachine.h"
#include "jsparser.h"
#include "jsparser_fsm.c"

#define state_external(x) jsparser_states_external[x]

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
    jsparser_ctx *js;

    def = statemachine_definition_new(JSPARSER_NUM_STATES);
    if(!def)
      return NULL;

    sm = statemachine_new(def);
    if(!sm)
      return NULL;

    js = calloc(1, sizeof(jsparser_ctx));
    if(!js)
      return NULL;

    js->statemachine = sm;
    js->statemachine_def = def;
    sm->user = js;

    statemachine_definition_populate(def, jsparser_state_transitions);

    return js;
}

void jsparser_reset(jsparser_ctx *ctx)
{
  assert(ctx);
  ctx->statemachine->current_state = 0;
}


int jsparser_state(jsparser_ctx *ctx)
{
  return state_external(ctx->statemachine->current_state);
}

int jsparser_parse(jsparser_ctx *ctx, const char *str, int size)
{
    int internal_state;
    internal_state = statemachine_parse(ctx->statemachine, str, size);
    return state_external(internal_state);
}

void jsparser_delete(jsparser_ctx *ctx)
{
    assert(ctx);
    statemachine_delete(ctx->statemachine);
    statemachine_definition_delete(ctx->statemachine_def);
    free(ctx);
}
