// Author: falmeida@google.com (Filipe Almeida)

#ifndef JSPARSER_H
#define JSPARSER_H

#include "statemachine.h"

enum js_state_external_enum {
    JSPARSER_STATE_TEXT,
    JSPARSER_STATE_Q,
    JSPARSER_STATE_DQ,
    JSPARSER_STATE_COMMENT,
};

typedef struct jsparser_ctx_s {
  statemachine_ctx *statemachine;
  statemachine_definition *statemachine_def;
} jsparser_ctx;

void jsparser_reset(jsparser_ctx *ctx);
jsparser_ctx *jsparser_new(void);
int jsparser_state(jsparser_ctx *ctx);
int jsparser_parse(jsparser_ctx *ctx, const char *str, int size);

void jsparser_delete(jsparser_ctx *ctx);

#define jsparser_parse_chr(a,b) jsparser_parse(a, &(b), 1);
#define jsparser_parse_str(a,b) jsparser_parse(a, b, strlen(b));

#endif /* JSPARSER_H */
