// Author: falmeida@google.com (Filipe Almeida)

#ifndef HTMLPARSER_H
#define HTMLPARSER_H

#include "statemachine.h"
#include "jsparser.h"

/* entity filter */

#define HTMLPARSER_MAX_STRING 256
#define HTMLPARSER_MAX_ENTITY_SIZE 10


enum htmlparser_state_external_enum {
    HTMLPARSER_STATE_TEXT,
    HTMLPARSER_STATE_TAG,
    HTMLPARSER_STATE_ATTR,
    HTMLPARSER_STATE_VALUE,
    HTMLPARSER_STATE_COMMENT,
    HTMLPARSER_STATE_JS_FILE,
    HTMLPARSER_STATE_CSS_FILE,
    HTMLPARSER_STATE_ERROR
};

enum htmlparser_mode {
    HTMLPARSER_MODE_HTML,
    HTMLPARSER_MODE_JS
};

enum htmlparser_attr_type {
    HTMLPARSER_ATTR_NONE,
    HTMLPARSER_ATTR_REGULAR,
    HTMLPARSER_ATTR_URI,
    HTMLPARSER_ATTR_JS,
    HTMLPARSER_ATTR_STYLE
};

typedef struct entityfilter_ctx_s {
    char buffer[HTMLPARSER_MAX_ENTITY_SIZE];
    int buffer_pos;
    int in_entity;
    char output[HTMLPARSER_MAX_ENTITY_SIZE];
} entityfilter_ctx;

void entityfilter_reset(entityfilter_ctx *ctx);
entityfilter_ctx *entityfilter_new(void);
const char *entityfilter_process(entityfilter_ctx *ctx, char c);


/* html parser */

typedef struct htmlparser_ctx_s {
  statemachine_ctx *statemachine;
  statemachine_definition *statemachine_def;
  jsparser_ctx *jsparser;
  entityfilter_ctx *entityfilter;
  int value_state;
  int value_index;
  char tag[HTMLPARSER_MAX_STRING];
  char attr[HTMLPARSER_MAX_STRING];
} htmlparser_ctx;

void htmlparser_reset(htmlparser_ctx *ctx);
void htmlparser_reset_mode(htmlparser_ctx *ctx, int mode);
htmlparser_ctx *htmlparser_new(void);
int htmlparser_state(htmlparser_ctx *ctx);
int htmlparser_parse(htmlparser_ctx *ctx, const char *str, int size);

const char *htmlparser_state_str(htmlparser_ctx *ctx);
int htmlparser_is_attr_quoted(htmlparser_ctx *ctx);
int htmlparser_in_js(htmlparser_ctx *ctx);

const char *htmlparser_tag(htmlparser_ctx *ctx);
const char *htmlparser_attr(htmlparser_ctx *ctx);

int htmlparser_js_state(htmlparser_ctx *ctx);
int htmlparser_is_js_quoted(htmlparser_ctx *ctx);
int htmlparser_value_index(htmlparser_ctx *ctx);
int htmlparser_attr_type(htmlparser_ctx *ctx);

void htmlparser_delete(htmlparser_ctx *ctx);

#define htmlparser_parse_chr(a,b) htmlparser_parse(a, &(b), 1);
#define htmlparser_parse_str(a,b) htmlparser_parse(a, b, strlen(b));


#endif /* HTMLPARSER_H */
