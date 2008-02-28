/* 
 * Author: falmeida@google.com (Filipe Almeida)
 */

/* TODO(falmeida): add handle_pi()
 * TODO(falmeida): Breaks on NULL characters in the stream. fix.
 * TODO(falmeida): Bound checks everywhere
 * TODO(falmeida): Comments
 * TODO(falmeida): Change function declaration to adapt to the style guide
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "change_defs.h"
#include "statemachine.h"
#include "htmlparser.h"
#include "htmlparser_fsm.c"
#include "jsparser.h"

#define is_js_attribute(attr) ((attr)[0] == 'o' && (attr)[1] == 'n')
#define is_style_attribute(attr) (strcmp((attr), "style") == 0)

enum value_state {
    VALUE_STATE_FIRST,
    VALUE_STATE_NEXT,
    VALUE_STATE_JS_FIRST,
    VALUE_STATE_JS_NEXT
};

/* html entity filter */
static struct entityfilter_table_s {
    const char *entity;
    const char *value;
} entityfilter_table[] = {
    { "lt",     "<" },
    { "gt",     ">" },
    { "quot",   "\"" },
    { "amp",    "&" },
    { "apos",   "\'" },
    { NULL,     NULL }
};

/* Utility functions */

/* Converts the internal state into the external superstate.
 */
static int state_external(int st) {
    if(st == STATEMACHINE_ERROR)
      return HTMLPARSER_STATE_ERROR;
    else
      return htmlparser_states_external[st];
}

/* Returns true if the attribute is expected to contain a url
 * This list was taken from: http://www.w3.org/TR/html4/index/attributes.html
 */
static int is_uri_attribute(char *attr)
{
  /* TODO(falmeida): Style. Google style guide says we need a space between a
   * statement and the parenteses, but I'm keeping this for now in order to be
   * consistent with the rest of the file.
   * Although this is meant to be open sourced and will live in third_party,
   * I'll make a cl that makes the style google friendly */

  if(!attr)
    return 0;

  switch(attr[0]) {
    case 'a':
      if(strcmp(attr, "action") == 0)
        return 1;
      /* TODO(falmeida): This is a uri list. Should we treat it diferently? */
      if(strcmp(attr, "archive") == 0) /* This is a uri list */
        return 1;
      break;

    case 'b':
      if(strcmp(attr, "background") == 0)
        return 1;
      break;

    case 'c':
      if(strcmp(attr, "cite") == 0)
        return 1;
      if(strcmp(attr, "classid") == 0)
        return 1;
      if(strcmp(attr, "codebase") == 0)
        return 1;
      break;

    case 'd':
      if(strcmp(attr, "data") == 0)
        return 1;
      if(strcmp(attr, "dynsrc") == 0) /* from msdn */
        return 1;
      break;

    case 'h':
      if(strcmp(attr, "href") == 0)
        return 1;
      break;

    case 'l':
      if(strcmp(attr, "longdesc") == 0)
        return 1;
      break;

    case 's':
      if(strcmp(attr, "src") == 0)
        return 1;
      break;

    case 'u':
      if(strcmp(attr, "usemap") == 0)
        return 1;
      break;
  }

  return 0;

}

/* Convert a string to lower case characters inplace.
 */
static void tolower_str(char *s)
{
    while(*s) {
      *s = (char)tolower(*s);
      s++;
    }
}

/* Resets the entityfilter to it's initial state so it can be reused.
 */
void entityfilter_reset(entityfilter_ctx *ctx)
{
    ctx->buffer[0] = 0;
    ctx->buffer_pos = 0;
    ctx->in_entity = 0;
}

/* Initializes a new entity filter object.
 */
entityfilter_ctx *entityfilter_new()
{
    entityfilter_ctx *ctx = malloc(sizeof(entityfilter_ctx));
    if(!ctx)
      return NULL;
    ctx->buffer[0] = 0;
    ctx->buffer_pos = 0;
    ctx->in_entity = 0;

    return ctx;
}

/* Deallocates an entity filter object.
 */
void entityfilter_delete(entityfilter_ctx *ctx)
{
    free(ctx);
}

/* Converts a string containing an hexadecimal number to a string containing
 * one character with the corresponding ascii value.
 *
 * The provided output char array must be at least 2 chars long.
 */
static const char *parse_hex(const char *s, char *output)
{
    int n;
    n = strtol(s, NULL, 16);
    output[0] = n;
    output[1] = 0;
    /* TODO(falmeida): Make this function return void */
    return output;
}

/* Converts a string containing a decimal number to a string containing one
 * character with the corresponding ascii value.
 *
 * The provided output char array must be at least 2 chars long.
 */
static const char *parse_dec(const char *s, char *output)
{
    int n;
    n = strtol(s, NULL, 10);
    output[0] = n;
    output[1] = 0;
    return output;
}

/* Converts a string with an html entity to it's encoded form, which is written
 * to the output string
 */
static const char *entity_convert(const char *s, char *output)
{
  /* TODO(falmeida): Handle wide char encodings */
  /* TODO(falmeida): validation missing */
  /* TODO(falmeida): bounds checking everywhere */
    struct entityfilter_table_s *t = entityfilter_table;

    if(s[0] == '#') {
      if(s[1] == 'x') { // hex
          return parse_hex(s + 2, output);
      } else { // decimal
          return parse_dec(s + 1, output);
      }
    }

    while(t->entity) {
        if(strcasecmp(t->entity, s) == 0)
            return t->value;
        t++;
    }

    snprintf(output, HTMLPARSER_MAX_ENTITY_SIZE, "&%s;", s);
    output[HTMLPARSER_MAX_ENTITY_SIZE - 1] = '\0';

    return output;
}


/* Processes a character from the input stream and decodes any html entities
 * in the processed input stream.
 *
 * Returns a reference to a string that points to an internal buffer. This
 * buffer will be changed after every call to entityfilter_process(). As
 * such this string should be duplicated before subsequent calls to
 * entityfilter_process().
 */
const char *entityfilter_process(entityfilter_ctx *ctx, char c)
{
    if(ctx->in_entity) {
        if(c == ';' || isspace(c)) {
            ctx->in_entity = 0;
            ctx->buffer[ctx->buffer_pos] = 0;
            ctx->buffer_pos = 0;
            return entity_convert(ctx->buffer, ctx->output);
        } else {
            ctx->buffer[ctx->buffer_pos++] = c;
            if(ctx->buffer_pos >= HTMLPARSER_MAX_ENTITY_SIZE - 1) {
                /* No more buffer to use, finalize and return */
                ctx->buffer[ctx->buffer_pos] = '\0';
                ctx->in_entity=0;
                ctx->buffer_pos = 0;
                strncpy(ctx->output, ctx->buffer, HTMLPARSER_MAX_ENTITY_SIZE);
                ctx->output[HTMLPARSER_MAX_ENTITY_SIZE - 1] = '\0';
                return ctx->output;
            }
        }
    } else {
        if(c == '&') {
            ctx->in_entity = 1;
            ctx->buffer_pos = 0;
        } else {
            ctx->output[0] = c;
            ctx->output[1] = 0;
            return ctx->output;
        }
    }
    return "";
}

/* Called when the parser enters a new tag. Starts recording it's name into
 * html->tag.
 */
static void enter_tag_name(statemachine_ctx *ctx, int start, char chr, int end)
{
    htmlparser_ctx *html = (htmlparser_ctx *)ctx->user;
    statemachine_start_record(ctx, html->tag, HTMLPARSER_MAX_STRING - 1);
}

/* Called when the parser exits the tag name in order to finalize the recording.
 *
 * It converts the tag name to lowercase, and if the tag was closed, just
 * clears html->tag.
 */
static void exit_tag_name(statemachine_ctx *ctx, int start, char chr, int end)
{
    htmlparser_ctx *html = (htmlparser_ctx *)ctx->user;
    statemachine_stop_record(ctx);

/* TODO(falmeida): If the parser is interrupted in the middle of a tag,
 * html->tag may not be null terminated or contain garbage. Same applies for
 * html->attr */

    tolower_str(html->tag);

    if(html->tag[0] == '/')
      html->tag[0] = 0;
}

/* Called when the parser enters a new tag. Starts recording it's name into
 * html->attr
 */
static void enter_attr(statemachine_ctx *ctx, int start, char chr, int end)
{
    htmlparser_ctx *html = (htmlparser_ctx *)ctx->user;
    statemachine_start_record(ctx, html->attr, HTMLPARSER_MAX_STRING - 1);
}

/* Called when the parser exits the tag name in order to finalize the recording.
 *
 * It converts the tag name to lowercase.
 */
static void exit_attr(statemachine_ctx *ctx, int start, char chr, int end)
{
    htmlparser_ctx *html = (htmlparser_ctx *)ctx->user;
    statemachine_stop_record(ctx);
    tolower_str(html->attr);
}

/* Called everytime the parser leaves a tag definition.
 *
 * For now it is only used to find script blocks.
 */
static void enter_text(statemachine_ctx *ctx, int start, char chr, int end)
{
    htmlparser_ctx *html = (htmlparser_ctx *)ctx->user;
    /* Handle CDATA sections. Only script tags for now */
    /* TODO(falmeida): add support for TEXTAREA */
    if(strcmp(html->tag, "script") == 0) {
      ctx->next_state = HTMLPARSER_STATE_INT_JS_TEXT;
      jsparser_reset(html->jsparser);
    }
}

/* Called inside script blocks in order to parse the javascript
 *
 * It currently assumes the script is a javascript file or a scripting language
 * with equivalent syntax for string literals and comments.
 */
static void in_state_js(statemachine_ctx *ctx, int start, char chr, int end)
{
  htmlparser_ctx *html = (htmlparser_ctx *)ctx->user;
  jsparser_parse_chr(html->jsparser, chr);
}

/* Called when we enter an attribute value.
 *
 * Keeps track of a position index inside the value and initializes the
 * javascript state machine for attributes that except javascript.
 */
static void enter_value(statemachine_ctx *ctx, int start, char chr, int end)
{
  htmlparser_ctx *html = (htmlparser_ctx *)ctx->user;
  html->value_index = 0;

  if(is_js_attribute(html->attr)) {
    html->value_state = VALUE_STATE_JS_FIRST;
    entityfilter_reset(html->entityfilter);
    jsparser_reset(html->jsparser);
  } else {
    html->value_state = VALUE_STATE_FIRST;
  }

}

/* Called when for every character inside a quoted attribute value
 *
 * Used to process javascript and keep track of the position index inside the
 * attribute value.
 */
static void in_state_value_quoted(statemachine_ctx *ctx, int start, char chr, int end)
{
  /*TODO(falmeida): The logic for this is a bit complex and could possibly be
   * simplified by changing the state machine. */
  htmlparser_ctx *html = (htmlparser_ctx *)ctx->user;
  int attr_state = html->value_state;
  const char *output;

  if(attr_state == VALUE_STATE_FIRST) {
    html->value_state = VALUE_STATE_NEXT;
    return;
  } else if (attr_state == VALUE_STATE_NEXT) {
    html->value_index++;
    return;
  }

  /* The first character is a " or a ', so we have to ignore it */
  if(attr_state == VALUE_STATE_JS_FIRST)
    html->value_state = VALUE_STATE_JS_NEXT;
  else {
    html->value_index++;
    output = entityfilter_process(html->entityfilter, chr);
    jsparser_parse_str(html->jsparser, output);
  }
}

/* Called when for every character inside an unquoted attribute value
 *
 * Used to process javascript and keep track of the position index inside the
 * attribute value.
 */
static void in_state_value_text(statemachine_ctx *ctx, int start, char chr, int end)
{
  htmlparser_ctx *html = (htmlparser_ctx *)ctx->user;
  int attr_state = html->value_state;
  const char *output = entityfilter_process(html->entityfilter, chr);

  html->value_index++;

  if(attr_state == VALUE_STATE_FIRST || attr_state == VALUE_STATE_NEXT)
    return;

  jsparser_parse_str(html->jsparser, output);
}

/* htmlparser_reset_mode(htmlparser_ctx *ctx, int mode)
 *
 * Resets the parser to it's initial state and changes the parser mode.
 * All internal context like tag name, attribute name or the state of the
 * statemachine are reset to it's original values as if the object was just
 * created.
 *
 * Available modes:
 *  HTMLPARSER_MODE_HTML - Parses html text
 *  HTMLPARSER_MODE_JS - Parses javascript files
 *
 */
void htmlparser_reset_mode(htmlparser_ctx *ctx, int mode)
{
  assert(ctx);
  ctx->statemachine->current_state = 0;
  ctx->tag[0] = '\0';
  ctx->attr[0] = '\0';
  jsparser_reset(ctx->jsparser);

  switch(mode) {
    case HTMLPARSER_MODE_HTML:
      ctx->statemachine->current_state = HTMLPARSER_STATE_INT_TEXT;
      break;
    case HTMLPARSER_MODE_JS:
      ctx->statemachine->current_state = HTMLPARSER_STATE_INT_JS_FILE;
      break;
  }
}

/* Resets the parser to it's initial state and to the default mode, which
 * is MODE_HTML.
 *
 * All internal context like tag name, attribute name or the state of the
 * statemachine are reset to it's original values as if the object was just
 * created.
 */
void htmlparser_reset(htmlparser_ctx *ctx)
{
    assert(ctx);
    htmlparser_reset_mode(ctx, HTMLPARSER_MODE_HTML);
}

/* Initializes a new htmlparser instance.
 *
 * Returns a pointer to the new instance or NULL if the initialization fails.
 * Initialization failure is fatal, and if this function fails it may not
 * deallocate all previsouly allocated memory.
 */
htmlparser_ctx *htmlparser_new()
{
    statemachine_ctx *sm;
    statemachine_definition *def;
    htmlparser_ctx *html;

    /* TODO(falmeida): We may want to share the same definition across multiple
     * instances in the future */
    def = statemachine_definition_new(HTMLPARSER_NUM_STATES);
    if(!def)
      return NULL;

    sm = statemachine_new(def);
    html = calloc(1, sizeof(htmlparser_ctx));
    if(!html)
      return NULL;

    html->statemachine = sm;
    html->statemachine_def = def;
    html->jsparser = jsparser_new();
    if(!html->jsparser)
      return NULL;

    html->entityfilter = entityfilter_new();
    if(!html->entityfilter)
      return NULL;

    html->tag[0] = 0;
    html->attr[0] = 0;
    sm->user = html;

    statemachine_definition_populate(def, htmlparser_state_transitions);

    statemachine_enter_state(def, HTMLPARSER_STATE_INT_TAG_NAME,
                             enter_tag_name);
    statemachine_exit_state(def, HTMLPARSER_STATE_INT_TAG_NAME, exit_tag_name);

    statemachine_enter_state(def, HTMLPARSER_STATE_INT_ATTR, enter_attr);
    statemachine_exit_state(def, HTMLPARSER_STATE_INT_ATTR, exit_attr);

    statemachine_enter_state(def, HTMLPARSER_STATE_INT_TEXT, enter_text);

/* javascript states */
    statemachine_in_state(def, HTMLPARSER_STATE_INT_JS_TEXT, in_state_js);
    statemachine_in_state(def, HTMLPARSER_STATE_INT_JS_COMMENT, in_state_js);
    statemachine_in_state(def, HTMLPARSER_STATE_INT_JS_COMMENT_OPEN,
                          in_state_js);
    statemachine_in_state(def, HTMLPARSER_STATE_INT_JS_COMMENT_IN,
                          in_state_js);
    statemachine_in_state(def, HTMLPARSER_STATE_INT_JS_COMMENT_CLOSE,
                          in_state_js);
    statemachine_in_state(def, HTMLPARSER_STATE_INT_JS_LT, in_state_js);
    statemachine_in_state(def, HTMLPARSER_STATE_INT_JS_FILE, in_state_js);

/* value states */
    statemachine_enter_state(def, HTMLPARSER_STATE_INT_VALUE, enter_value);
    statemachine_in_state(def, HTMLPARSER_STATE_INT_VALUE_TEXT,
                          in_state_value_text);
    statemachine_in_state(def, HTMLPARSER_STATE_INT_VALUE_Q,
                          in_state_value_quoted);
    statemachine_in_state(def, HTMLPARSER_STATE_INT_VALUE_DQ,
                          in_state_value_quoted);

    return html;
}

/* Receives an htmlparser context and Returns the current html state.
 */
int htmlparser_state(htmlparser_ctx *ctx)
{
  return state_external(ctx->statemachine->current_state);
}

/* Parses the input html stream and returns the finishing state.
 *
 * Returns HTMLPARSER_ERROR if unable to parse the input. If htmlparser_parse()
 * is called after an error situation was encountered the behaviour is
 * unspecified. At this point, htmlparser_reset() or htmlparser_reset_mode()
 * can be called to reset the state.
 */
int htmlparser_parse(htmlparser_ctx *ctx, const char *str, int size)
{
    int internal_state;
    internal_state = statemachine_parse(ctx->statemachine, str, size);
    return state_external(internal_state);
}


/* Returns true if the parser is inside an attribute value and the value is
 * surrounded by single or double quotes. */
int htmlparser_is_attr_quoted(htmlparser_ctx *ctx) {
  int st = ctx->statemachine->current_state;
  if(st == HTMLPARSER_STATE_INT_VALUE_Q ||
     st == HTMLPARSER_STATE_INT_VALUE_DQ)
      return 1;
  else
      return 0;
}

/* Returns true if the parser is currently in javascript. This can be a
 * an attribute that takes javascript, a javascript block or the parser
 * can just be in MODE_JS. */
int htmlparser_in_js(htmlparser_ctx *ctx) {
  int st = ctx->statemachine->current_state;

  if(st == HTMLPARSER_STATE_INT_JS_TEXT ||
     st == HTMLPARSER_STATE_INT_JS_COMMENT ||
     st == HTMLPARSER_STATE_INT_JS_COMMENT_OPEN ||
     st == HTMLPARSER_STATE_INT_JS_COMMENT_IN ||
     st == HTMLPARSER_STATE_INT_JS_COMMENT_CLOSE ||
     st == HTMLPARSER_STATE_INT_JS_LT ||
     st == HTMLPARSER_STATE_INT_JS_FILE)
    return 1;

  if(state_external(st) == HTMLPARSER_STATE_VALUE &&
     ctx->attr[0] == 'o' &&
     ctx->attr[1] == 'n')
      return 1;
  else
      return 0;
}

/* Returns the current tag or NULL if not available.
 *
 * There is no stack implemented because we currently don't have a need for
 * it, which means tag names are tracked only one level deep.
 *
 * This is better understood by looking at the following example:
 *
 * <b [tag=b]>
 *   [tag=b]
 *   <i>
 *    [tag=i]
 *   </i>
 *  [tag=NULL]
 * </b>
 *
 * The tag is correctly filled inside the tag itself and before any new inner
 * tag is closed, at which point the tag will be null.
 *
 * For our current purposes this is not a problem, but we may implement a tag
 * tracking stack in the future for completeness.
 *
 */
const char *htmlparser_tag(htmlparser_ctx *ctx)
{
  /* TODO(falmeida): This will blow up in something like <scr<?cs blah?>>, so we
   * have to check for statemachine->buffer_pos */
  if(ctx->tag[0] != 0)
    return ctx->tag;
  else
    return NULL;
}

/* Returns true if inside an attribute or a value */
int htmlparser_in_attr(htmlparser_ctx *ctx)
{
    int ext_state = state_external(ctx->statemachine->current_state);
    return (ext_state == HTMLPARSER_STATE_ATTR || ext_state == HTMLPARSER_STATE_VALUE);
}

/* Returns the current attribute name if inside an attribute name or an
 * attribute value. Returns NULL otherwise */
const char *htmlparser_attr(htmlparser_ctx *ctx)
{
  if(htmlparser_in_attr(ctx))
    return ctx->attr;
  else
    return NULL;
}

/* Returns the current state of the javascript state machine
 *
 * Currently only present for testing purposes.
 */
int htmlparser_js_state(htmlparser_ctx *ctx)
{
   return jsparser_state(ctx->jsparser);
}

/* True is currently inside a javascript string literal
 */
int htmlparser_is_js_quoted(htmlparser_ctx *ctx)
{
    if(htmlparser_in_js(ctx)) {
      int st = jsparser_state(ctx->jsparser);
      if(st == JSPARSER_STATE_Q ||
         st == JSPARSER_STATE_DQ)
        return 1;
    }
    return 0;
}

/* True if currently inside an attribute value
 */
int htmlparser_in_value(htmlparser_ctx *ctx)
{
    int ext_state = state_external(ctx->statemachine->current_state);
    return ext_state == HTMLPARSER_STATE_VALUE;
}

/* Returns the position inside the current attribute value
 */
int htmlparser_value_index(htmlparser_ctx *ctx)
{
    if(htmlparser_in_value(ctx))
        return ctx->value_index;

    return -1;
}

/* Returns the current attribute type.
 *
 * The attribute type can be one of:
 *   HTMLPARSER_ATTR_NONE - not inside an attribute.
 *   HTMLPARSER_ATTR_REGULAR - Inside a normal attribute.
 *   HTMLPARSER_ATTR_URI - Inside an attribute that accepts a uri.
 *   HTMLPARSER_ATTR_JS - Inside a javascript attribute.
 *   HTMLPARSER_ATTR_STYLE - Inside a css style attribute.
 */
int htmlparser_attr_type(htmlparser_ctx *ctx)
{
    if(!htmlparser_in_attr(ctx))
        return HTMLPARSER_ATTR_NONE;

    if(is_js_attribute(ctx->attr))
        return HTMLPARSER_ATTR_JS;

    if(is_uri_attribute(ctx->attr))
        return HTMLPARSER_ATTR_URI;

    if(is_style_attribute(ctx->attr))
        return HTMLPARSER_ATTR_STYLE;

    return HTMLPARSER_ATTR_REGULAR;
}

/* Deallocates an htmlparser context object.
 */
void htmlparser_delete(htmlparser_ctx *ctx)
{
    assert(ctx);
    statemachine_definition_delete(ctx->statemachine_def);
    statemachine_delete(ctx->statemachine);
    jsparser_delete(ctx->jsparser);
    entityfilter_delete(ctx->entityfilter);
    free(ctx);
}
