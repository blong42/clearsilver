/*
 * All Rights Reserved.
 *
 * ClearSilver Templating System
 *
 * This code is made available under the terms of the ClearSilver License.
 * http://www.clearsilver.net/license.hdf
 *
 */

#ifndef __NEO_AUTO_H_
#define __NEO_AUTO_H_ 1

__BEGIN_DECLS

#include "htmlparser/htmlparser.h"

/* Currently neos_auto_ctx does not have any members other than the
 *  htmlparser_ctx object. This may change in the future.
 */
typedef htmlparser_ctx NEOS_AUTO_CTX;

/*
 * Function: neos_auto_escape - Escape input according to auto-escape context.
 * Description: neos_auto_escape takes an auto-escape context, determines the
 *              escaping modifications needed to make input safe in that context.
 *              It applies these modifications to the input string. It will
 *              first determine if any modifications are necessary, if not - the
 *              input string is returned as output.
 * Input: ctx -> an object specifying the currrent auto-escape context.
 *        str -> input string which will be escaped.
 *
 * Output: esc -> the escaped string will be returned in this pointer.
 *         do_free -> if *do_free is true, *esc should be freed. If not, *esc
 *         contains str and should not be freed.
 * Returns: NERR_NOMEM if unable to allocate memory for output.
 *          NERR_ASSERT if any of the supplied pointers are NULL.
 */
NEOERR *neos_auto_escape(NEOS_AUTO_CTX *ctx, const char* str,
                         char **esc, int *do_free);

/*
 * Function: neos_auto_parse - Parse input and update auto-escape context.
 * Description: neos_auto_parse takes an auto-escape context, which contains a
 *              pointer to the underlying htmlparser. It passes the input string
 *              to the htmlparser, which updates its internal state accordingly.
 * Input: ctx -> an object specifying the currrent auto-escape context.
 *        str -> input string to parse.
 *        len -> length of str.
 *
 * Output: None
 *
 * Returns: NERR_ASSERT if ctx or str is NULL.
 */
NEOERR *neos_auto_parse(NEOS_AUTO_CTX *ctx, const char *str, int len);

/*
 * Function: neos_auto_init - Create and initialize a NEOS_AUTO_CTX object.
 * Description: Returns an initialized NEOS_AUTO_CTX object, by internally
 *              initializing an htmlparser object.
 * Input: pctx -> pointer to pointer to NEOS_AUTO_CTX structure.
 *
 * Output: pctx -> will contain the newly initialized NEOS_AUTO_CTX object.
 * Returns: NERR_NOMEM if unable to allocate memory for *pctx.
 *          NERR_ASSERT if pctx is NULL.
 */
NEOERR *neos_auto_init(NEOS_AUTO_CTX **pctx);

/*
 * Function: neos_auto_destroy - Free the NEOS_AUTO_CTX object.
 * Description: Calls htmlparser_delete to free the associated htmlparser object.
 * Input: pctx -> pointer to pointer that should be freed.
 *
 * Output: None.
 *
 * Returns: None.
 */
void neos_auto_destroy(NEOS_AUTO_CTX **pctx);

/*
 * Function: neos_auto_set_content_type - Sets expected content type of input.
 * Description: Takes a MIME type header, and configures the underlying
 *              htmlparser object to expect input of the corresponding type.
 *              This functionality is provided for situations where the html
 *              parser cannot itself determine the proper starting context of
 *              the input, for instance - when a javascript source file or a
 *              stylesheet is being parsed.
 *              This function should be called before any input is processed
 *              using neos_auto_parse().
 * Input: ctx -> an object specifying the currrent auto-escape context.
 *        type -> the MIME type of input. Currently supported MIME types are:
 *               "text/html", "text/xml", "text/plain", "application/xhtml+xml",
 *               "application/javascript", "application/json", "text/javascript"
 *
 * Output: None.
 *
 * Returns: NERR_ASSERT for unknown content type.
 */
NEOERR *neos_auto_set_content_type(NEOS_AUTO_CTX *ctx, const char *type);

__END_DECLS

#endif /* __NEO_AUTO_H_ */
