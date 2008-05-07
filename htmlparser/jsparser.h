/* Copyright 2007 Google Inc. All Rights Reserved.
 * Author: falmeida@google.com (Filipe Almeida)
 */

#ifndef __NEO_JSPARSER_H_
#define __NEO_JSPARSER_H_

#include "statemachine.h"

#ifdef __cplusplus
namespace security_streamhtmlparser {
#endif /* __cplusplus */

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
#ifdef __cplusplus
#define jsparser_parse_str(a,b) jsparser_parse(a, b, \
                                               static_cast<int>(strlen(b)));
#else
#define jsparser_parse_str(a,b) jsparser_parse(a, b, (int)strlen(b));
#endif

#ifdef __cplusplus
}  /* namespace security_streamhtmlparser */
#endif /* __cplusplus */

#endif /* __NEO_JSPARSER_H_ */
