/*
 * All Rights Reserved.
 *
 * ClearSilver Templating System
 *
 * This code is made available under the terms of the ClearSilver License.
 * http://www.clearsilver.net/license.hdf
 *
 */

#include <ctype.h>
#include <string.h>
#include "streamhtmlparser/htmlparser.h"
#include "neo_err.h"
#include "neo_str.h"
#include "neo_auto.h"

struct _neos_auto_ctx {
  htmlparser_ctx *hctx;
};

/* This structure is used to map an HTTP content type to the htmlparser mode
 * that should be used for parsing it.
 */
static struct _neos_content_map
{
  char *content_type;  /* mime type of input */
  int parser_mode;  /* corresponding htmlparser mode */
} ContentTypeList[] = {
  {"none", -1},
  {"text/html", HTMLPARSER_MODE_HTML},
  {"text/plain", HTMLPARSER_MODE_HTML},
  {"application/javascript", HTMLPARSER_MODE_JS},
  {"application/json", HTMLPARSER_MODE_JS},
  {"text/javascript", HTMLPARSER_MODE_JS},
  {"text/css", HTMLPARSER_MODE_CSS},
  {NULL, -1},
};

/* Characters to escape when html escaping is needed */
static char *HTML_CHARS_LIST    = "&<>\"'\r";

/* Characters to escape when html escaping an unquoted
   attribute value */
static char *HTML_UNQUOTED_LIST = "&<>\"'=";

/* Characters to escape when javascript escaping is needed */
static char *JS_CHARS_LIST       = "&<>\"'\r\n\t/\\;";

/* Characters to escape when unquoted javascript attribute is escaped */
static char *JS_ATTR_UNQUOTED_LIST = "&<>\"'/\\;=\t\n\v\f\r ";


/*  The html escaping routine uses this map to lookup the appropriate
 *  entity encoding to use for any metacharacter.
 *  A filler "&nbsp;" has been used for characters that we do not expect to
 *  lookup.
 */
static char* HTML_CHAR_MAP[] =
  {"&nbsp;", "&nbsp;", "&nbsp;", "&nbsp;", "&nbsp;",
   "&nbsp;", "&nbsp;", "&nbsp;", "&nbsp;", "&nbsp;",
   "&nbsp;", "&nbsp;", "&nbsp;", "&nbsp;", "&nbsp;",
   "&nbsp;", "&nbsp;", "&nbsp;", "&nbsp;", "&nbsp;",
   "&nbsp;", "&nbsp;", "&nbsp;", "&nbsp;", "&nbsp;",
   "&nbsp;", "&nbsp;", "&nbsp;", "&nbsp;", "&nbsp;",
   "&nbsp;", "&nbsp;", "&nbsp;", "!" ,     "&quot;",
   "#",      "$",      "%",      "&amp;",  "&#39;",
   "(", ")", "*", "+", ",", "-", ".", "/", "0", "1",
   "2", "3", "4", "5", "6", "7", "8", "9", ":", ";",
   "&lt;", "&#61;", "&gt;", "?", "@", "A", "B", "C",
   "D", "E",
   "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
   "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y",
   "Z", "[", "\\", "]", "^", "_", "`", "a", "b", "c",
   "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
   "n", "o", "p", "q", "r", "s", "t", "u", "v", "w",
   "x", "y", "z", "{", "|", "}", "~", "&nbsp;",};

/* Characters that are safe to use inside CSS context */
static char *CSS_SAFE_CHARS = "_.,!#%-";

#define IN_LIST(l, c) ( ((unsigned char)c < 0x80) && (strchr(l, c) != NULL) )

#define IS_SPACE(c) ( (c == ' ' || c == '\t' || c == '\n' || \
                       c == '\v' || c == '\f' || c == '\r') )

/* All control chars except the following space characters defined by HTML 5.0:
   space(0x20), tab(0x9), line feed(0xa), vertical tab(0xb), form feed(0xc),
   return(0xd)
*/
#define IS_CTRL_CHAR(c) ( (c > 0x00 && c <= 0x08) || (c >= 0x0e && c <= 0x1f) \
                          || (c == 0x7f) )

/* neo_str.c already has all these escaping routines.
 * But since with auto escaping, the escaping routines will be called for every
 * variable, these new functions do an initial pass to determine if escaping is
 * needed. If not, the input string itself is returned, and no extraneous
 * memory allocation or copying is done.
 */

/* TODO(mugdha): Consolidate these functions with the ones in neo_str.c. */

/* Function: neos_auto_html_escape - HTML escapes input if necessary.
 * Description: This function scans through in, looking for HTML metacharacters.
 *              If any metacharacters are found, the input is HTML escaped and
 *              the resulting output returned in *esc.
 * Input: in -> input string
 *        quoted -> should be 0 if the input string appears on an HTML attribute
 *                  and is not quoted.
 * Output: esc -> pointer to output string. Could point back to input string
 *                if the input does not need modification.
 *         do_free -> will be 1 if *esc should be freed. If it is 0, *esc
 *                    points to in.
 */
static NEOERR *neos_auto_html_escape (const char *in, char **esc,
                                      int quoted, int *do_free)
{
  unsigned int newlen = 0;
  int modify = 0;
  unsigned int l = 0;
  char *tmp = NULL;
  char *metachars = HTML_CHARS_LIST;

  *do_free = 0;

  if (!quoted)
    metachars = HTML_UNQUOTED_LIST;

  /*
     Check if there are any characters that need escaping. In the majority of
     cases, this will be false and we can just quit the function immediately
  */
  while (in[l])
  {
    if (!quoted && (IS_CTRL_CHAR(in[l]) || IS_SPACE(in[l])))
    {
      modify = 1;
    }
    else
    {
      if (IN_LIST(metachars, in[l]))
      {
        newlen += strlen(HTML_CHAR_MAP[(int)in[l]]);
        modify = 1;
      }
      else
        newlen += 1;
    }

    l++;
  }

  if (!modify) {
    *esc = (char *)in;
    return STATUS_OK;
  }

  /*
     There are some HTML metacharacters. Allocate a new string and do escaping
     in that string.
  */
  (*esc) = (char *) malloc(newlen + 1);
  if (*esc == NULL)
    return nerr_raise (NERR_NOMEM, "Unable to allocate memory to escape %s",
                       in);
  *do_free = 1;

  tmp = *esc;
  l = 0;
  while (in[l])
  {
    if (!quoted && (IS_CTRL_CHAR(in[l]) || IS_SPACE(in[l])))
    {
      /* Ignore this character */
    }
    else
    {
      if (IN_LIST(metachars, in[l]))
      {
        strncpy(tmp, HTML_CHAR_MAP[(int)in[l]],
                strlen(HTML_CHAR_MAP[(int)in[l]]));
        tmp += strlen(HTML_CHAR_MAP[(int)in[l]]);
      }
      else
      {
        *tmp++ = in[l];
      }
    }
    l++;
  }
  (*esc)[newlen] = '\0';

  return STATUS_OK;
}

/* Allowed URI schemes, used by neos_auto_url_validate. If a provided URL
 * has any other scheme, it will be rejected.
 */
static char *AUTO_URL_PROTOCOLS[] = {"http://", "https://", "ftp://", "mailto:"};

/* Function: neos_auto_url_validate - Verify that the input is a valid URL.
 * Description: This function verifies that the input starts with one of the
 *              allowed URI schemes, specified in AUTO_URL_PROTOCOLS.
 *              In addition, the input is html escaped.
 * Input: in -> input string
 *        quoted -> should be 0 if the input string appears on an HTML attribute
 *                  and is not quoted. Will be passed through to the html
 *                  escaping function.
 * Output: esc -> pointer to output string. Could point back to input string
 *                if the input does not need modification.
 *         do_free -> will be 1 if *esc should be freed. If it is 0, *esc
 *                    points to in.
 */
static NEOERR *neos_auto_url_validate (const char *in, char **esc,
                                       int quoted, int *do_free)
{
  int valid = 0;
  size_t i;
  size_t inlen;
  int num_protocols = sizeof(AUTO_URL_PROTOCOLS) / sizeof(char*);
  char* slashpos;
  void* colonpos;
  char *s;

  *do_free = 0;
  inlen = strlen(in);

  /*
   * <a href="//b:80"> or <a href="a/b:80"> are allowed by browsers
   * and ":" is treated as part of the path, while
   * <a href="www.google.com:80"> is an invalid url
   * and ":" is treated as a scheme separator.
   *
   * Hence allow for ":" in the path part of a url (after /)
   */
  slashpos = strchr(in, '/');
  if (slashpos == NULL) {
    i = inlen;
  }
  else {
    i = (size_t)(slashpos - in);
  }

  colonpos = memchr(in, ':', i);

  if (colonpos == NULL) {
    /* no scheme in 'in': so this is a relative url */
    valid = 1;
  }
  else {
    for (i = 0; i < num_protocols; i++)
    {
      if (strncasecmp(in, AUTO_URL_PROTOCOLS[i], strlen(AUTO_URL_PROTOCOLS[i]))
          == 0) {
        /* 'in' starts with one of the allowed protocols */
        valid = 1;
        break;
      }

    }
  }

  if (valid)
    return nerr_pass(neos_auto_html_escape(in, esc, quoted, do_free));

  /* 'in' contains an unsupported scheme, replace with '#' */
  s = (char *) malloc(2);
  if (s == NULL)
    return nerr_raise (NERR_NOMEM,
                       "Unable to allocate memory to escape %s", in);

  s[0] = '#';
  s[1] = '\0';
  *esc = s;
  *do_free = 1;
  return STATUS_OK;

}

/* Function: neos_auto_check_number - Verify that in points to a number.
 * Description: This function scans through in and validates that it contains
 *              a number. Digits, decimal points and spaces are ok. If the
 *              input is not a valid number, output is set to "null". A pointer
 *              to the output is returned in *esc.
 * Input: in -> input string
 *
 * Output: esc -> pointer to output string. Will point back to input string
 *                if the input is a valid number. Otherwise, points to '_'.
 *         do_free -> will be 1 if *esc should be freed. If it is 0, *esc
 *                    points to in.
 */
static NEOERR *neos_auto_check_number (const char *in, char **esc, int *do_free)
{
  size_t i;
  int inlen;
  int valid;

  *do_free = 0;

  inlen = strlen(in);
  valid = 1;

  /* Permit boolean literals */
  if ((strcmp(in, "true") == 0) || (strcmp(in, "false") == 0)) {
    *esc = (char *)in;
    return STATUS_OK;
  }

  if (in[0] == '0' && inlen > 2 && (in[1] == 'x' || in[1] == 'X')) {
    /* There must be at least one hex digit after the 0x for it to be valid.
       Hex number. Check that it is of the form 0(x|X)[0-9A-Fa-f]+
    */
    for (i = 2; i < inlen; i++) {
      if (!((in[i] >= 'a' && in[i] <= 'f') ||
            (in[i] >= 'A' && in[i] <= 'F') ||
            (in[i] >= '0' && in[i] <= '9'))) {
        valid = 0;
        break;
      }
    }
  } else {
    /* Must be a base-10 (or octal) number.
       Check that it has the form [0-9+-.eE]+
    */
    for (i = 0; i < inlen; i++) {
      if (!((in[i] >= '0' && in[i] <= '9') ||
            in[i] == '+' || in[i] == '-' || in[i] == '.' ||
            in[i] == 'e' || in[i] == 'E')) {
        valid = 0;
        break;
      }
    }
  }

  if (!valid) {
    char *tmp_esc;
    tmp_esc = (char *) malloc(5);
    if (tmp_esc == NULL)
      return nerr_raise (NERR_NOMEM,
                         "Unable to allocate memory to escape %s", in);

    strcpy(tmp_esc, "null");
    *esc = tmp_esc;
    *do_free = 1;
  }
  else
    *esc = (char *)in;

  return STATUS_OK;
}

/* Function: neos_auto_css_validate - Verify that in points to safe css subset.
 * Description: This function verifies that 'in' points to a safe subset of
 *              characters that are ok to use as the value of a style property.
 *              Alphanumeric characters, space (0x20), non-ascii characters and
 *              _.,!#%- are allowed.
 *              All other characters are stripped out. A pointer to the
 *              output is returned in *esc.
 * Input: in -> input string
 *
 * Output: esc -> pointer to output string. Will point back to input string
 *                if the input is not modified.
 *         do_free -> will be 1 if *esc should be freed. If it is 0, *esc
 *                    points to in.
 */
static NEOERR *neos_auto_css_validate (const unsigned char *in, char **esc,
                                       int quoted, int *do_free)
{
  int l = 0;

  *do_free = 0;
  while (in[l] &&
         (isalnum(in[l]) || (in[l] == ' ' && quoted) || 
          IN_LIST(CSS_SAFE_CHARS, in[l]) || in[l] >= 0x80)) {
    l++;
  }

  if (!in[l]) {
    /* while() looped successfully through all characters in 'in'.
       'in' is safe to use as is */
    *esc = (char *)in;
  }
  else {
    /* Create a new string, stripping out all dangerous characters from 'in' */
    int i = 0;
    char *s;
    s = (char *) malloc(strlen(in) + 1);
    if (s == NULL)
      return nerr_raise (NERR_NOMEM,
                         "Unable to allocate memory to escape %s", in);

    l = 0;
    while (in[l]) {
      /* Strip out all except a whitelist of characters */
      if (isalnum(in[l]) || (in[l] == ' ' && quoted) ||
          IN_LIST(CSS_SAFE_CHARS, in[l]) || in[l] >= 0x80) {
        s[i++] = in[l];
      }

      l++;
    }
    s[i] = '\0';
    *esc = s;
    *do_free = 1;
  }

  return STATUS_OK;
}

/* Function: neos_auto_js_escape - Javascript escapes input if necessary.
 * Description: This function scans through in, looking for javascript
 *              metacharacters. If any are found, the input is js escaped and
 *              the resulting output returned in *esc.
 * Input: in -> input string
 *        attr_quoted -> should be 0 if the input string appears on an JS
 *                       attribute and the entire attribute is not quoted.
 * Output: esc -> pointer to output string. Could point back to input string
 *                if the input does not need modification.
 *         do_free -> will be 1 if *esc should be freed. If it is 0, *esc
 *                    points to in.
 */
static NEOERR *neos_auto_js_escape (const char *in, char **esc,
                                    int attr_quoted, int *do_free)
{
  int nl = 0;
  int l = 0;
  char *s;
  char *metachars = JS_CHARS_LIST;

  *do_free = 0;
  /*
    attr_quoted can be false if
    - a variable inside a javascript attribute is being escaped
    - AND, the variable itself is quoted, but the entire attribute is not.
    <input onclick=alert('<?cs var: Blah ?>');>
    The variable could be used to inject additional attributes on the tag.
  */
  if (!attr_quoted)
    metachars = JS_ATTR_UNQUOTED_LIST;

  while (in[l])
  {
    if (IN_LIST(metachars, in[l]) || (in[l] > 0 && in[l] < 32) ||
        (in[l] == 0x7f))
    {
      nl += 3;
    }
    l++;
  }

  if (nl == 0) {
    *esc = (char *)in;
    return STATUS_OK;
  }

  nl += l;
  s = (char *) malloc(nl + 1);
  if (s == NULL)
    return nerr_raise (NERR_NOMEM, "Unable to allocate memory to escape %s",
                       in);

  l = 0;
  nl = 0;
  while (in[l])
  {
    if (IN_LIST(metachars, in[l]) || (in[l] > 0 && in[l] < 32) ||
        (in[l] == 0x7f))
    {
      s[nl++] = '\\';
      s[nl++] = 'x';
      s[nl++] = "0123456789ABCDEF"[(in[l] >> 4) & 0xF];
      s[nl++] = "0123456789ABCDEF"[in[l] & 0xF];
      l++;
    }
    else
    {
      s[nl++] = in[l++];
    }
  }
  s[nl] = '\0';

  *esc = (char *)s;
  *do_free = 1;
  return STATUS_OK;
}

/* Function: neos_auto_tag_validate - Force in to valid tag or attribute name.
 * Description: This function scans through in, looking for characters that are
 *              illegal in a tag or attribute name. It uses the same regular
 *              expression as the htmlparser - [A-Za-z0-9_:-]. All other
 *              characters are stripped out.
 * Input: in -> input string
 *
 * Output: esc -> pointer to output string. Could point back to input string
 *                if the input does not need modification.
 *         do_free -> will be 1 if *esc should be freed. If it is 0, *esc
 *                    points to in.
 */
static NEOERR *neos_auto_tag_validate (const char *in, char **esc, int *do_free)
{
  int l = 0;

  *do_free = 0;
  while (in[l] &&
         (isalnum(in[l]) || in[l] == ':' || in[l] == '_' || in[l] == '-')) {
    l++;
  }

  if (!in[l]) {
    /* while() looped successfully through all characters in 'in'.
       'in' is safe to use as is */
    *esc = (char *)in;
  }
  else {
    /* Create a new string, stripping out all dangerous characters from 'in' */
    int i = 0;
    char *s;
    s = (char *) malloc(strlen(in) + 1);
    if (s == NULL)
      return nerr_raise (NERR_NOMEM,
                         "Unable to allocate memory to escape %s", in);

    l = 0;
    while (in[l]) {
      /* Strip out all except a whitelist of characters */
      if (isalnum(in[l]) || in[l] == ':' || in[l] == '_' || in[l] == '-') {
        s[i++] = in[l];
      }

      l++;
    }

    s[i] = '\0';
    *esc = s;
    *do_free = 1;
  }

  return STATUS_OK;
}

NEOERR *neos_auto_escape(NEOS_AUTO_CTX *ctx, const char* str, char **esc,
                         int *do_free)
{
  htmlparser_ctx *hctx;
  int st;
  const char *tag;

  if (!ctx)
    return nerr_raise(NERR_ASSERT, "ctx is NULL");

  if (!str)
    return nerr_raise(NERR_ASSERT, "str is NULL");

  if (!esc)
    return nerr_raise(NERR_ASSERT, "esc is NULL");

  if (!do_free)
    return nerr_raise(NERR_ASSERT, "do_free is NULL");

  hctx = ctx->hctx;
  st = htmlparser_state(hctx);
  tag = htmlparser_tag(hctx);

  /* Inside an HTML tag or attribute name */
  if (st == HTMLPARSER_STATE_ATTR || st == HTMLPARSER_STATE_TAG) {
    return nerr_pass(neos_auto_tag_validate(str, esc, do_free));
  }

  /* Inside an HTML attribute value */
  if (st == HTMLPARSER_STATE_VALUE) {

    int type = htmlparser_attr_type(hctx);
    int attr_quoted = htmlparser_is_attr_quoted(hctx);

    switch (type) {
      case HTMLPARSER_ATTR_REGULAR:
        /* <input value="<?cs var: Blah ?>"> : */
        return nerr_pass(neos_auto_html_escape(str, esc,
                                               attr_quoted, do_free));
        
      case HTMLPARSER_ATTR_URI:
        if (htmlparser_value_index(hctx) == 0)
          /* <a href="<?cs var:MyUrl ?>"> : Validate URI scheme of MyUrl */
          return nerr_pass(neos_auto_url_validate(str, esc,
                                                  attr_quoted, do_free));
        else
          /* <a href="http://www.blah.com?x=<?cs var: MyQuery ?>">:
            MyQuery is not at start of URL, so it only needs html escaping.
          */
          return nerr_pass(neos_auto_html_escape(str, esc,
                                                 attr_quoted, do_free));

      case HTMLPARSER_ATTR_JS:
        if (htmlparser_is_js_quoted(hctx))
          /* <input onclick="alert('<?cs var:Blah ?>');"> OR
             <input onclick=alert('<?cs var: Blah ?>');>
          */
          /*
            Note: neos_auto_js_escape() hex encodes all html metacharacters.
            Therefore it is safe to not do an HTML escape around this.
          */
          return nerr_pass(neos_auto_js_escape(str, esc, attr_quoted,
                                               do_free));
        else
          /* <input onclick="alert(<?cs var:Blah ?>);"> OR
             <input onclick=alert(<?cs var:Blah ?>);> :
            
            There are no quotes around the variable, it could be used to
            inject arbitrary javascript. Only reason to omit the quotes is if
            the variable is intended to be a number.
          */
          return nerr_pass(neos_auto_check_number(str, esc, do_free));
        break;

      case HTMLPARSER_ATTR_STYLE:
        /* <input style="border:<?cs var: FancyBorder ?>"> : */
        return nerr_pass(neos_auto_css_validate((unsigned char*)str, esc,
                                                attr_quoted, do_free));

      default:
        return nerr_raise(NERR_ASSERT, 
                          "Unknown attr type received from HTML parser : %d\n",
                          type);
    }
  }

  if (st == HTMLPARSER_STATE_CSS_FILE || 
      (st == HTMLPARSER_STATE_TEXT && tag && strcmp(tag, "style") == 0)) {
    return nerr_pass(neos_auto_css_validate(str, esc,
                                            1, do_free));
  }

  /* Inside javascript. Do JS escaping */
  if (htmlparser_in_js(hctx)) {
    if (htmlparser_is_js_quoted(hctx))
      /* <script> var a = <?cs var: Blah ?>; </script> */
      /* TODO(mugdha): This also includes variables inside javascript comments.
         They will also get stripped out if they are not numbers.
      */
      return nerr_pass(neos_auto_js_escape(str, esc, 1, do_free));
    else
      /* <script> var a = "<?cs var: Blah ?>"; </script> */
      return nerr_pass(neos_auto_check_number(str, esc, do_free));
  }

  /* Default is assumed to be HTML body */
  /* <b>Hello <?cs var: UserName ?></b> : */
  return nerr_pass(neos_auto_html_escape(str, esc, 1, do_free));

}

NEOERR *neos_auto_parse_var(NEOS_AUTO_CTX *ctx, const char *str, int len)
{
  int st;
  int retval;

  if (!ctx)
    return nerr_raise(NERR_ASSERT, "ctx is NULL");

  if (!str)
    return nerr_raise(NERR_ASSERT, "str is NULL");

  st = htmlparser_state(ctx->hctx);
  /*
   * Do not parse variables outside a tag declaration because
   * - they are unlikely to affect html parser state or
   * - they may contain user controlled html that could confuse the parser.
   */
  if ((st == HTMLPARSER_STATE_VALUE) ||
      (st == HTMLPARSER_STATE_ATTR) ||
      (st == HTMLPARSER_STATE_TAG)) {
    /* TODO(mugdha): This condition matches start of tag <<?cs var: TagName ?>>,
       but not end of tag </<?cs var: TagName ?>>.
       This will be a problem if variables are used for tags we care about:
       i.e. script, style, title, textarea.
    */
    retval = htmlparser_parse(ctx->hctx, str, len);
    if (retval == HTMLPARSER_STATE_ERROR)
      return nerr_raise(NERR_ASSERT,
                        "HTML Parser failed to parse : %s",
                        str);
  }

  return STATUS_OK;
}

NEOERR *neos_auto_parse(NEOS_AUTO_CTX *ctx, const char *str, int len)
{
  int retval;

  if (!ctx)
    return nerr_raise(NERR_ASSERT, "ctx is NULL");

  if (!str)
    return nerr_raise(NERR_ASSERT, "str is NULL");

  retval = htmlparser_parse(ctx->hctx, str, len);
  if (retval == HTMLPARSER_STATE_ERROR)
  {
    if (len < 200)
    {
      return nerr_raise(NERR_ASSERT,
                        "HTML Parser failed to parse : %s",
                        str);
    }
    else
    {
      return nerr_raise(
          NERR_ASSERT,
          "HTML Parser failed to parse string(length %d) starting with: %.200s",
          len, str);
    }
  }

  return STATUS_OK;
}

NEOERR *neos_auto_set_content_type(NEOS_AUTO_CTX *ctx, const char *type)
{
  struct _neos_content_map *esc;

  if (!ctx)
    return nerr_raise(NERR_ASSERT, "ctx is NULL");

  if (!type)
    return nerr_raise(NERR_ASSERT, "type is NULL");

  for (esc = &ContentTypeList[0]; esc->content_type != NULL; esc++) {
    if (strcmp(type, esc->content_type) == 0) {
        htmlparser_reset_mode(ctx->hctx, esc->parser_mode);
        return STATUS_OK;
    }
  }

  return nerr_raise(NERR_ASSERT,
                    "unknown content type supplied: %s", type);

}

NEOERR* neos_auto_init(NEOS_AUTO_CTX **pctx)
{
  NEOERR *err = STATUS_OK;

  if (!pctx)
    return nerr_raise(NERR_ASSERT, "pctx is NULL");

  *pctx = (NEOS_AUTO_CTX *) calloc (1, sizeof(NEOS_AUTO_CTX));
  if (*pctx == NULL)
    return nerr_raise(NERR_NOMEM, "Could not create autoescape context");

  (*pctx)->hctx = htmlparser_new();

  if ((*pctx)->hctx == NULL)
    err = nerr_raise(NERR_NOMEM, "Could not create autoescape context");

  return err;

}

NEOERR* neos_auto_reset(NEOS_AUTO_CTX *ctx)
{
  NEOERR *err = STATUS_OK;

  if (!ctx)
    return nerr_raise(NERR_ASSERT, "ctx is NULL");

  if (ctx->hctx)
    htmlparser_reset(ctx->hctx);
  else
  {
    ctx->hctx = htmlparser_new();
    if (ctx->hctx == NULL)
      err = nerr_raise(NERR_NOMEM, "Could not create htmlparser context");
  }

  return err;
}

void neos_auto_destroy(NEOS_AUTO_CTX **pctx)
{
  if (!pctx)
    return;

  if (*pctx) {
    if ((*pctx)->hctx)
      htmlparser_delete((*pctx)->hctx);
    free(*pctx);
  }
  *pctx = NULL;
}
