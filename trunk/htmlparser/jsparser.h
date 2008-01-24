// Author: falmeida@google.com (Filipe Almeida)

#ifndef JSPARSER_H
#define JSPARSER_H

#include "statemachine.h"
#include "jsparser_states.h"

typedef struct jsparser_ctx_s {
  statemachine_ctx *statemachine;
  statemachine_definition *statemachine_def;
} jsparser_ctx;

void jsparser_reset(jsparser_ctx *ctx);
jsparser_ctx *jsparser_new(void);
state jsparser_state(jsparser_ctx *ctx);
state jsparser_parse(jsparser_ctx *ctx, const char *str, int size);

const char *jsparser_state_str(jsparser_ctx *ctx);

void jsparser_delete(jsparser_ctx *ctx);

#define jsparser_parse_chr(a,b) jsparser_parse(a, &(b), 1);
#define jsparser_parse_str(a,b) jsparser_parse(a, b, strlen(b));

#endif /* JSPARSER_H */
