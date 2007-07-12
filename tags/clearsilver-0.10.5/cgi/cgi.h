/*
 * Copyright 2001-2004 Brandon Long
 * All Rights Reserved.
 *
 * ClearSilver Templating System
 *
 * This code is made available under the terms of the ClearSilver License.
 * http://www.clearsilver.net/license.hdf
 *
 */

#ifndef __CGI_H_ 
#define __CGI_H_ 1

#include <stdarg.h>
#include "util/neo_err.h"
#include "util/neo_hdf.h"
#include "cs/cs.h"

__BEGIN_DECLS

extern NERR_TYPE CGIFinished;
extern NERR_TYPE CGIUploadCancelled;
extern NERR_TYPE CGIParseNotHandled;

/* HACK: Set this value if you want to treat empty CGI Query variables as
 * non-existant.
 */
extern int IgnoreEmptyFormVars;

typedef struct _cgi CGI;

typedef int (*UPLOAD_CB)(CGI *, int nread, int expected);
typedef NEOERR* (*CGI_PARSE_CB)(CGI *, char *method, char *ctype, void *rock);

struct _cgi_parse_cb
{
  char *method;
  int any_method;
  char *ctype;
  int any_ctype;
  void *rock;
  CGI_PARSE_CB parse_cb;
  struct _cgi_parse_cb *next;
};

struct _cgi
{
  /* Only public parts of this structure */
  void *data;  /* you can store your own information here */
  HDF *hdf;    /* the HDF dataset associated with this CGI */

  BOOL ignore_empty_form_vars;

  UPLOAD_CB upload_cb;

  int data_expected;
  int data_read;
  struct _cgi_parse_cb *parse_callbacks;

  /* For line oriented reading of form-data input.  Used during cgi_init
   * only */
  char *buf;
  int buflen;
  int readlen;
  BOOL found_nl;
  BOOL unget;
  char *last_start;
  int last_length;
  int nl;

  /* this is a list of filepointers pointing at files that were uploaded */
  /* Use cgi_filehandle to access these */
  ULIST *files;

  /* By default, cgi_parse unlinks uploaded files as it opens them. */
  /* If Config.Upload.Unlink is set to 0, the files are not unlinked */
  /* and there names are stored in this list. */
  /* Use Query.*.FileName to access these */
  ULIST *filenames;

  /* keep track of the time between cgi_init and cgi_render */
  double time_start;
  double time_end;
};


/*
 * Function: cgi_init - Initialize ClearSilver CGI environment
 * Description: cgi_init initializes the ClearSilver CGI environment,
 *              including creating the HDF data set.  It will then import 
 *              the standard CGI environment variables into that dataset,
 *              will parse the QUERY_STRING into the data set, and parse
 *              the HTTP_COOKIE into the data set.  Note that if the
 *              var xdisplay is in the form data, cgi_init will attempt
 *              to validate the value and launch the configured debugger
 *              on the CGI program.  These variables have to be
 *              specified in the hdf_file pointed to by hdf_file.  The
 *              default settings do not allow debugger launching for
 *              security reasons.
 * Input: cgi - a pointer to a CGI pointer
 *        hdf_file - the path to an HDF data set file that will also be
 *                   loaded into the dataset.  This will likely have to
 *                   a be a full path, as the HDF search paths are not
 *                   yet set up.  Certain things, like 
 * Output: cgi - an allocated CGI struct, including 
 * Return: NERR_PARSE - parse error in CGI input
 *         NERR_NOMEM - unable to allocate memory
 *         NERR_NOT_FOUND - hdf_file doesn't exist
 */
NEOERR *cgi_init (CGI **cgi, HDF *hdf);

/*
 * Function: cgi_parse - Parse incoming CGI data
 * Description: We split cgi_init into two sections, one that parses
 * 		just the basics, and the second is cgi_parse.  cgi_parse
 * 		is responsible for parsing the entity body of the HTTP
 * 		request.  This payload is typically only sent (expected)
 * 		on POST/PUT requests, but generally this is called on
 * 		all incoming requests.  This function walks the list of
 * 		registered parse callbacks (see cgi_register_parse_cb),
 * 		and if none of those matches or handles the request, it
 * 		falls back to the builtin handlers: 
 * 		  POST w/ application/x-www-form-urlencoded 
 * 		  POST w/ application/form-data
 * 		  PUT w/ any content type
 * 		In general, if there is no Content-Length, then
 * 		cgi_parse ignores the payload and doesn't raise an
 * 		error.
 * Input: cgi - a pointer to a CGI pointer
 * Output: Either data populated into files and cgi->hdf, or whatever
 *         other side effects of your own registered callbacks.
 * Return: NERR_PARSE - parse error in CGI input
 *         NERR_NOMEM - unable to allocate memory
 *         NERR_NOT_FOUND - hdf_file doesn't exist
 *         NERR_IO - error reading HDF file or reading CGI stdin, or
 *                   writing data on multipart/form-data file submission
 *         Anything else you raise.
 */
NEOERR *cgi_parse (CGI *cgi);

/*
 * Function: cgi_register_parse_cb - Register a parse callback
 * Description: The ClearSilver CGI Kit has built-in functionality to handle 
 *              the following methods:
 *              GET -> doesn't have any data except query string, which
 *                is processed for all methods
 *              POST w/ application/x-www-form-urlencoded
 *              POST w/ multipart/form-data
 *                processed as RFC2388 data into files and HDF (see
 *                cgi_filehandle())
 *              PUT (any type)
 *                The entire data chunk is stored as a file, with meta
 *                data in HDF (similar to single files in RFC2388). 
 *                The data is accessible via cgi_filehandle with NULL
 *                for name.
 *              To handle other methods/content types, you have to
 *              register your own parse function.  This isn't necessary
 *              if you aren't expecting any data, and technically HTTP
 *              only allows data on PUT/POST requests (and presumably
 *              user defined methods).  In particular, if you want to
 *              implement XML-RPC or SOAP, you'll have to register a
 *              callback here to grab the XML data chunk.  Usually
 *              you'll want to register POST w/ application/xml or POST
 *              w/ text/xml (you either need to register both or
 *              register POST w/ * and check the ctype yourself,
 *              remember to nerr_raise(CGIParseNotHandled) if you aren't
 *              handling the POST).
 *              In general, your callback should:
 *                Find out how much data is available:
 *                 l = hdf_get_value (cgi->hdf, "CGI.ContentLength", NULL); 
 *                 len = atoi(l);
 *                And read/handle all of the data using cgiwrap_read.
 *                See the builtin handlers for how this is done.  Note
 *                that cgiwrap_read is not guarunteed to return all of
 *                the data you request (just like fread(3)) since it
 *                might be reading of a socket.  Sorry.
 *                You should be careful when reading the data to watch
 *                for short reads (ie, end of file) and cases where the
 *                client sends you data ad infinitum.
 * Input: cgi - a CGI struct
 *        method - the HTTP method you want to handle, or * for all
 *        ctype - the HTTP Content-Type you want to handle, or * for all
 *        rock - opaque data that we'll pass to your call back
 * Output: None
 * Return: CGIParseNotHandled if your callback doesn't want to handle
 *         this.  This causes cgi_parse to continue walking the list of
 *         callbacks.
 *
 */
NEOERR *cgi_register_parse_cb(CGI *cgi, const char *method, const char *ctype,
                              void *rock, CGI_PARSE_CB parse_cb);

/*
 * Function: cgi_destroy - deallocate the data associated with a CGI
 * Description: cgi_destroy will destroy all the data associated with a
 *              CGI, which mostly means the associated HDF and removal
 *              of any files that were uploaded via multipart/form-data.
 *              (Note that even in the event of a crash, these files
 *              will be deleted, as they were unlinked on creation and
 *              only exist because of the open file pointer)
 * Input: cgi - a pointer to a pointer to a CGI struct
 * Output: cgi - NULL on output
 * Return: None
 */
void cgi_destroy (CGI **cgi);

/*
 * Function: cgi_cs_init - initialize CS parser with the CGI defaults
 * Description: cgi_cs_init initializes a CS parser with the CGI HDF
 *              context, and registers the standard CGI filters
 * Input: cgi - a pointer a CGI struct allocated with cgi_init
 *        cs - a pointer to a CS struct pointer
 * Output: cs - the allocated/initialized CS struct
 * Return: NERR_NOMEM - no memory was available to render the template
 */
NEOERR *cgi_cs_init(CGI *cgi, CSPARSE **cs);

/*
 * Function: cgi_display - render and display the CGI output to the user
 * Description: cgi_display will render the CS template pointed to by 
 *              cs_file using the CGI's HDF data set, and send the
 *              output to the user.  Note that the output is actually
 *              rendered into memory first.
 * Input: cgi - a pointer a CGI struct allocated with cgi_init
 *        cs_file - a ClearSilver template file
 * Output: None
 * Return: NERR_IO - an IO error occured during output
 *         NERR_NOMEM - no memory was available to render the template
 */
NEOERR *cgi_display (CGI *cgi, const char *cs_file);

/*
 * Function: cgi_output - display the CGI output to the user
 * Description: Normally, this is called by cgi_display, but some
 *              people wanted it external so they could call it
 *              directly.
 * Input: cgi - a pointer a CGI struct allocated with cgi_init
 *        output - the data to send to output from the CGI
 * Output: None
 * Return: NERR_IO - an IO error occured during output
 *         NERR_NOMEM - no memory was available to render the template
 */
NEOERR *cgi_output (CGI *cgi, STRING *output);

/*
 * Function: cgi_filehandle - return a file pointer to an uploaded file
 * Description: cgi_filehandle will return the stdio FILE pointer
 *              associated with a file that was uploaded using
 *              multipart/form-data.  The FILE pointer is positioned at
 *              the start of the file when first available.
 * Input: cgi - a pointer to a CGI struct allocated with cgi_init
 *        form_name - the form name that the file was uploaded as
 *                    (not the filename) (if NULL, we're asking for the
 *                    file handle for the PUT upload)
 * Output: None
 * Return: A stdio FILE pointer, or NULL if an error occurs (usually
 *         indicates that the form_name wasn't found, but might indicate
 *         a problem with the HDF dataset)
 */
FILE *cgi_filehandle (CGI *cgi, const char *form_name);

/*
 * Function: cgi_neo_error - display a NEOERR call backtrace
 * Description: cgi_neo_error will output a 500 error containing the
 *              NEOERR call backtrace.  This function is likely to be
 *              removed from future versions in favor of some sort of
 *              user error mechanism.
 * Input: cgi - a pointer to a CGI struct
 *        err - a NEOERR (see util/neo_err.h for details)
 * Output: None
 * Return: None
 */
void cgi_neo_error (CGI *cgi, NEOERR *err);

/*
 * Function: cgi_error - display an error string to the user
 * Description: cgi_error will output a 500 error containing the
 *              specified error message.  This function is likely to be
 *              removed from future versions in favor of a user error
 *              mechanism.
 * Input: cgi - a pointer to a CGI struct
 *        fmt - printf style format string and arguments
 * Output: None
 * Return: None
 */
void cgi_error (CGI *cgi, const char *fmt, ...)
                ATTRIBUTE_PRINTF(2,3);

/*
 * Function: cgi_debug_init - initialize standalone debugging
 * Description: cgi_debug_init initializes a CGI program for standalone
 *              debugging.  By running a ClearSilver CGI program with a
 *              filename on the command line as the first argument, the
 *              CGI program will load that file of the form K=V as a set
 *              of HTTP/CGI environment variables.  This allows you to
 *              run the program under a debugger in a reproducible
 *              environment.
 * Input: argc/argv - the arguments from main
 * Output: None
 * Return: None
 */
void cgi_debug_init (int argc, char **argv);

/*
 * Function: cgi_url_escape - url escape a string
 * Description: cgi_url_escape will do URL escaping on the passed in
 *              string, and return a newly allocated string that is escaped.
 *              Characters which are escaped include control characters,
 *              %, ?, +, space, =, &, /, and "
 * Input: buf - a 0 terminated string
 * Output: esc - a newly allocated string 
 * Return: NERR_NOMEM - no memory available to allocate the escaped string
 */
NEOERR *cgi_url_escape (const char *buf, char **esc);

/*
 * Function: cgi_url_escape_more - url escape a string
 * Description: cgi_url_escape_more will do URL escaping on the passed in
 *              string, and return a newly allocated string that is escaped.
 *              Characters which are escaped include control characters,
 *              %, ?, +, space, =, &, /, and " and any characters in
 *              other
 * Input: buf - a 0 terminated string
 *        other - a 0 terminated string of characters to escape
 * Output: esc - a newly allocated string 
 * Return: NERR_NOMEM - no memory available to allocate the escaped string
 */
NEOERR *cgi_url_escape_more (const char *buf, char **esc, const char *other);

/*
 * Function: cgi_url_validate - validate that url is of an allowed format
 * Description: cgi_url_validate will check that a URL starts with 
 *              one of the accepted safe schemes. 
 *              If not, it returns "#" as a safe substitute.
 *              Currently accepted schemes are http, https, ftp and mailto.
 *              It then html escapes the entire URL so that it is safe to
 *              insert in an href attribute.
 * Input: buf - a 0 terminated string
 * Output: esc - a newly allocated string 
 * Return: NERR_NOMEM - no memory available to allocate the escaped string
 */
NEOERR *cgi_url_validate (const char *buf, char **esc);

/*
 * Function: cgi_url_unescape - unescape an url encoded string
 * Description: cgi_url_unescape will do URL unescaping on the passed in
 *              string.  This function modifies the string in place
 *              This function will decode any %XX character, and will
 *              decode + as space
 * Input: buf - a 0 terminated string
 * Return: pointer to same buf
 */
char *cgi_url_unescape (char *buf);

/*
 * Function: cgi_redirect - send an HTTP 302 redirect response
 * Description: cgi_redirect will redirect the user to another page on
 *              your site.  This version takes only the path portion of
 *              the URL.  As with all printf style commands, you should
 *              not call this with arbitrary input that may contain %
 *              characters, if you are forwarding something directly,
 *              use a format like cgi_redirect (cgi, "%s", buf)
 * Input: cgi - cgi struct
 *        fmt - printf style format with args
 * Output: None
 * Return: None
 */
void cgi_redirect (CGI *cgi, const char *fmt, ...)
                   ATTRIBUTE_PRINTF(2,3);

/*
 * Function: cgi_redirect_uri - send an HTTP 302 redirect response
 * Description: cgi_redirect_uri will redirect the user to another page on
 *              your site.  This version takes the full URL, including
 *              protocol/domain/port/path.
 *              As with all printf style commands, you should
 *              not call this with arbitrary input that may contain %
 *              characters, if you are forwarding something directly,
 *              use a format like cgi_redirect (cgi, "%s", buf)
 * Input: cgi - cgi struct
 *        fmt - printf style format with args
 * Output: None
 * Return: None
 */
void cgi_redirect_uri (CGI *cgi, const char *fmt, ...)
                       ATTRIBUTE_PRINTF(2,3);

/*
 * Function: cgi_vredirect - send an HTTP 302 redirect response
 * Description: cgi_vredirect is mostly used internally, but can be used
 *              if you need a varargs version of the function.
 * Input: cgi - cgi struct
 *        uri - whether the URL is full (1) or path only (0)
 *        fmt - printf format string
 *        ap - stdarg va_list
 * Output: None
 * Return: None
 */
void cgi_vredirect (CGI *cgi, int uri, const char *fmt, va_list ap);


/*
 * Function: cgi_cookie_authority - determine the cookie authority for a
 *            domain
 * Description: cgi_cookie_authority will walk the CookieAuthority
 *              portion of the CGI HDF data set, and return the matching
 *              domain if it exists.  The purpose of this is so that you
 *              set domain specific cookies.  For instance, you might
 *              have
 *                CookieAuthority.0 = neotonic.com
 *              In which case, any webserver using a hostname ending in
 *              neotonic.com will generate a cookie authority of
 *              neotonic.com.
 * Input: cgi - a CGI struct
 *        host - optional host to match against.  If NULL, the function
 *               will use the HTTP.Host HDF variable.
 * Output: None
 * Return: The authority domain, or NULL if none found. 
 */
char *cgi_cookie_authority (CGI *cgi, const char *host);

/*
 * Function: cgi_cookie_set - Set a browser Cookie
 * Description: cgi_cookie_set will issue a Set-Cookie header that
 *              should cause a browser to return a cookie when required.
 *              Note this function does no escaping of anything, you
 *              have to take care of that first.
 * Input: cgi - a CGI struct
 *        name - the name of the cookie
 *        value - the value to set the cookie to.  
 *        path - optional path for which the cookie is valid.  Default
 *               is /
 *        domain - optional domain for which the cookie is valid.  You
 *                 can use cgi_cookie_authority to determine this
 *                 domain.  Default is none, which is interpreted by
 *                 the browser as the sending domain only.
 *        time_str - expiration time string in the following format
 *                   Wdy, DD-Mon-YYYY HH:MM:SS GMT.  Only used if
 *                   persistent.  Default is one year from time of call.
 *        persistent - cookie will be stored by the browser between sessions
 *        secure - cookie will only be sent over secure connections
 * Output: None
 * Return: NERR_IO
 */
NEOERR *cgi_cookie_set (CGI *cgi, const char *name, const char *value, 
                        const char *path, const char *domain, 
                        const char *time_str, int persistent, int secure);

/*
 * Function: cgi_cookie_clear - clear browser cookie
 * Description: cgi_cookie_clear will send back a Set-Cookie string that
 *              will attempt to stop a browser from continuing to send
 *              back a cookie.  Note that the cookie has to match in
 *              name, domain, and path, and the luck of the Irish has to
 *              be with you for this work all the time, but at the least
 *              it will make the browser send back a cookie with no
 *              value, which the ClearSilver cookie parsing code will
 *              ignore.
 * Input: cgi - a CGI struct
 *        name - the cookie name to clear
 *        domain - the domain to clear, NULL for none
 *        path - the cookie's path
 * Output: None
 * Return: NERR_IO
 */
NEOERR *cgi_cookie_clear (CGI *cgi, const char *name, const char *domain, 
                          const char *path);

/* not documented *yet* */
NEOERR *cgi_text_html_strfunc(const char *str, char **ret);
NEOERR *cgi_html_strip_strfunc(const char *str, char **ret);
NEOERR *cgi_html_escape_strfunc(const char *str, char **ret);
NEOERR *cgi_js_escape (const char *buf, char **esc);
void cgi_html_ws_strip(STRING *str, int level);
NEOERR *cgi_register_strfuncs(CSPARSE *cs);

/* internal use only */
NEOERR * parse_rfc2388 (CGI *cgi);
NEOERR * open_upload(CGI *cgi, int unlink_files, FILE **fpw);

__END_DECLS

#endif /* __CGI_H_ */
