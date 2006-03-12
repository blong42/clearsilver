/*
mod_ecs - Embedded ClearSilver CGI Apache Module

mod_ecs is a heavily modified version of mod_ecgi from:
http://www.webthing.com/software/mod_ecgi.html

This version is designed to run with the ClearSilver CGIKit, specifically 
with the cgi_wrap calls from that kit.  Those calls wrap the standard CGI 
access methods, namely environment variables and stdin/stdout, allowing 
those calls to be replaced easily.  mod_ecs provides replacement calls which
interface directly with the Apache internals.

Additionally, mod_ecs is designed to dlopen() the shared library CGI once, 
and keep it in memory, making the CGI almost identical in performance to a 
regular Apache module.  The fact that your CGI will be called multiple times
is the biggest difference you can expect from a standard ClearSilver based CGI.
This means your code must be clean!

ECS - Embedded ClearSilver

Platform: UNIX only.  Anyone who wants to is welcome to port it elsewhere.

=======================================================
To COMPILE Apache with embedded CGI support, use
	-ldl in EXTRA_LIBS
	possibly -rdynamic in EXTRA_LFLAGS
 I took this out of the config because its not there on freebsd4 
 = ConfigStart
	LIBS="$LIBS -ldl"
 = ConfigEnd
(or as required by your platform)

OK, here's for APACI:
 * MODULE-DEFINITION-START
 * Name: ecs_module
 * MODULE-DEFINITION-END

=======================================================

=======================================================
BUGS
Lots - here are some obvious ones
	- won't work with NPH
	- No mechanism is provided for running from an SSI
	- Can't take part in content-negotiation
	- No graceful cleanup if a CGI program crashes (though it's OK
	  if the CGI fails but returns).
	- Suspected memory leak inherited from Apache (which ignores it
	  because it happens just before exit there).

*/

#include <dlfcn.h>
#include "mod_ecs.h"

#include "httpd.h"
#include "http_config.h"
#include "http_request.h"
#include "http_core.h"
#include "http_protocol.h"
#include "http_main.h"
#include "http_log.h"
#include "util_script.h"
#include "http_conf_globals.h"

module ecs_module;

/* Configuration stuff */

#define log_reason(reason,name,r) ap_log_error(APLOG_MARK,APLOG_ERR,(r)->server,(reason),(name))
#define log_scripterror(r,conf,ret,error) (log_reason((error),(r)->filename,(r)),ret)

char** ecs_create_argv(pool*,char*,char*,char*,char*,const char*);

/****************************************************************
 *
 * Actual CGI handling...
 */
const int ERROR = 500;
const int INTERNAL_REDIRECT = 3020;

#undef ECS_DEBUG

/******************************************************************
 * cgiwrap routines
 *   We've replaced all the normal CGI api calls with calls to the 
 *   appropriate cgiwrap routines instead.  Then, we provide versions of
 *   the cgiwrap callback here that interface directly with apache.  We
 *   need to mimic a bunch of the stuff that apache does in mod_cgi in
 *   order to implement the output portion of the CGI spec.
 */
typedef struct header_buf {
  char *buf;
  int len;
  int max;
  int loc;
  int nonl;
} HEADER_BUF;

typedef struct wrap_data {
  HEADER_BUF hbuf;
  int end_of_header;
  int returns;
  request_rec *r;
} WRAPPER_DATA;

static int buf_getline (const char *idata, int ilen, char *odata, int olen, int *nonl)
{
  char *eol;
  int len;

  *nonl = 1;
  eol = strchr (idata, '\n');
  if (eol == NULL)
  {
    len = ilen;
  }
  else
  {
    *nonl = 0;
    len = eol - idata + 1;
  }
  if (len > olen) len = olen;
  memcpy (odata, idata, len);
  odata[len] = '\0';
  return len;
}

static int h_getline (char *buf, int len, void *h)
{
  HEADER_BUF *hbuf = (HEADER_BUF *)h;
  int ret;

  buf[0] = '\0';
  if (hbuf->loc > hbuf->len)
    return 0;

  ret = buf_getline (hbuf->buf + hbuf->loc, hbuf->len - hbuf->loc, buf, len, &(hbuf->nonl));
  hbuf->loc += ret;
#if ECS_DEBUG>1
  fprintf (stderr, "h_getline: [%d] %s\n", ret, buf);
#endif
  return ret;
}

static int header_write (HEADER_BUF *hbuf, const char *data, int dlen)
{
  char buf[1024];
  int done, len;
  int nonl = hbuf->nonl;

  done = 0;
  while (done < dlen)
  {
    nonl = hbuf->nonl;
    len = buf_getline (data + done, dlen - done, buf, sizeof(buf), &(hbuf->nonl));
    if (len == 0)
      break;
    done += len;
    if (hbuf->len + len > hbuf->max)
    {
      hbuf->max *= 2;
      if (hbuf->len + len > hbuf->max)
      {
	hbuf->max += len + 1;
      }
      hbuf->buf = (char *) realloc ((void *)(hbuf->buf), hbuf->max);
    }
    memcpy (hbuf->buf + hbuf->len, buf, len);
    hbuf->len += len;
    if (!nonl && (buf[0] == '\n' || buf[0] == '\r'))
    {
      /* end of headers */
      return done;
    }
  }

  return 0;
}

/* The normal CGI module passes the returned data through
 * ap_scan_script_header().  We can't do that directly, since we don't
 * have a constant stream of data, so we buffer the header into our own
 * structure, and call ap_scan_script_header_err_core() with our own
 * getline() function to walk the header buffer we have.  We could
 * probably get some speed improvement by keeping the header buffer
 * between runs, instead of growing it every time... for later.  Also,
 * we currently don't use the pool allocation routines here, so we have
 * to be very careful not to leak.  We could probably at least use the
 * ap_register_cleanup() function to make sure we clean up our mess...
 */
static int wrap_write (void *data, const char *buf, size_t len)
{
  WRAPPER_DATA *wrap = (WRAPPER_DATA *)data;
  int wl;
  int ret;

#if ECS_DEBUG>1
  fprintf (stderr, "wrap_write (%s, %d)\n", buf, len);
#endif
  if (!wrap->end_of_header)
  {
    wl = header_write (&(wrap->hbuf), buf, len);
    if (wl == 0)
    {
      return len;
    }
    wrap->end_of_header = 1;
    wrap->hbuf.loc = 0;
#if ECS_DEBUG>1
    fprintf (stderr, "ap_scan_script_header_err_core\n%s\n", wrap->hbuf.buf);
#endif
    wrap->returns = ap_scan_script_header_err_core(wrap->r, NULL, h_getline, 
	(void *)&(wrap->hbuf));
#if ECS_DEBUG>1
    fprintf (stderr, "ap_scan_script_header_err_core.. done\n");
#endif
    if (len >= wl)
    {
      len = len - wl;
      buf = buf + wl;
    }

    if (wrap->returns == OK)
    {
      const char* location = ap_table_get (wrap->r->headers_out, "Location");

      if (location && location[0] == '/' && wrap->r->status == 200) 
      {
	wrap->returns = INTERNAL_REDIRECT;
      } 
      else if (location && wrap->r->status == 200) 
      {
	/* XX Note that if a script wants to produce its own Redirect
	 * body, it now has to explicitly *say* "Status: 302"
	 */
	wrap->returns = REDIRECT;
      } 
      else 
      {
#ifdef ECS_DEBUG
	fprintf (stderr, "ap_send_http_header\n");
#endif
	ap_send_http_header(wrap->r);
#ifdef ECS_DEBUG
	fprintf (stderr, "ap_send_http_header.. done\n");
#endif
      }
    }
  }
  /* if header didn't return OK, ignore the rest */
  if ((wrap->returns != OK) || wrap->r->header_only)
  {
    return len;
  }
#if ECS_DEBUG>1
  fprintf (stderr, "ap_rwrite(%s,%d)\n", buf, len);
#endif
  ret = ap_rwrite (buf, len, wrap->r);
#if ECS_DEBUG>1
  fprintf (stderr, "ap_rwrite.. done\n");
#endif
  return ret;
}

int wrap_vprintf (void *data, const char *fmt, va_list ap)
{
  char buf[4096];
  int len;

  len = ap_vsnprintf (buf, sizeof(buf), fmt, ap);
  return wrap_write (data, buf, len);
}

static int wrap_read (void *data, char *buf, size_t len)
{
  WRAPPER_DATA *wrap = (WRAPPER_DATA *)data;
  int ret;
  int x = 0;

#if ECS_DEBUG>1
  fprintf (stderr, "wrap_read (%s, %d)\n", buf, len);
#endif
  do
  {
    ret = ap_get_client_block(wrap->r, buf + x, len - x);
    if (ret <= 0) break;
    x += ret;
  } while (x < len);
#if ECS_DEBUG>1
  fprintf (stderr, "done ap_get_client_block\n");
#endif
  if (ret < 0) return ret;
  return x;
}

static char *wrap_getenv (void *data, const char *s)
{
  WRAPPER_DATA *wrap = (WRAPPER_DATA *)data;
  char *v;

  v = (char *) ap_table_get (wrap->r->subprocess_env, s);
  if (v) return strdup(v);
  return NULL;
}

static int wrap_putenv (void *data, const char *k, const char *v)
{
  WRAPPER_DATA *wrap = (WRAPPER_DATA *)data;

  ap_table_set (wrap->r->subprocess_env, k, v);

  return 0;
}	

static char *wrap_iterenv (void *data, int x, char **k, char **v)
{
  WRAPPER_DATA *wrap = (WRAPPER_DATA *)data;
  array_header *env = ap_table_elts(wrap->r->subprocess_env);
  table_entry *entry = (table_entry*)env->elts;

  if (x >= env->nelts) return 0;

  if (entry[x].key == NULL || entry[x].val == NULL)
    return 0;

  *k = strdup(entry[x].key);
  *v = strdup(entry[x].val);

  return 0;
}	

/*************************************************************************
 * Actual mod_ecs data structures for configuration
 */

typedef void (*InitFunc)();
typedef void (*CleanupFunc)();
typedef int (*CGIMainFunc)(int,char**,char**);
typedef int (*WrapInitFunc)(void *,void *,void*,void*,void*,void*,void*);

typedef struct {
  const char *libpath;
  ap_os_dso_handle_t dlib;
} ecs_deplibs;

typedef struct {
  const char *libpath;
  ap_os_dso_handle_t dlib;
  WrapInitFunc wrap_init;
  CGIMainFunc start;
  time_t mtime;
  int loaded;
} ecs_manager;

typedef struct {
  array_header *deplibs;
  array_header *handlers;
  int fork_enabled;
  int reload_enabled;
} ecs_server_conf;

const char *ECSInit = "ECSInit";
const char *ECSCleanUp = "ECSCleanup";
const char *WrapInit = "cgiwrap_init_emu";
const char *CGIMain = "main";

static void dummy (ap_os_dso_handle_t dlhandle)
{
}

static void slib_cleanup (ap_os_dso_handle_t dlhandle)
{
  CleanupFunc cleanupFunc;
  if ((cleanupFunc = (CleanupFunc)ap_os_dso_sym(dlhandle, ECSCleanUp))) {
    (*cleanupFunc)();
  }
  ap_os_dso_unload(dlhandle);
#ifdef ECS_DEBUG
  fprintf(stderr, "Unloading handle %d", dlhandle);
#endif
}

void *create_ecs_config (pool *p, server_rec *dummy)
{
  ecs_server_conf *new = ap_palloc (p, sizeof(ecs_server_conf));
  new->deplibs = ap_make_array(p,1,sizeof(ecs_deplibs));
  new->handlers = ap_make_array(p,1,sizeof(ecs_manager));
  new->fork_enabled = 0;
  new->reload_enabled = 0;
  return (void *) new;
}

char** e_setup_cgi_env (request_rec* r)
{
  char** env;

  ap_add_common_vars(r);
  ap_add_cgi_vars(r);
  env = ap_create_environment(r->pool,r->subprocess_env);

  return env;
}

const char *set_dep_lib (cmd_parms *parms, void *dummy, char *arg)
{
  ecs_server_conf *cls = ap_get_module_config (parms->server->module_config,
      &ecs_module);
  ecs_deplibs *entry;
  ap_os_dso_handle_t dlhandle;
  InitFunc init_func;

  if ((dlhandle = ap_os_dso_load(arg)) == NULL) {
    return ap_os_dso_error();
  }

  if ((init_func = (InitFunc)ap_os_dso_sym(dlhandle, ECSInit))) {
    (*init_func)();
  }

  ap_register_cleanup (cls->deplibs->pool, dlhandle, slib_cleanup, slib_cleanup);

  entry = (ecs_deplibs*)ap_push_array(cls->deplibs);
  entry->libpath = ap_pstrdup(cls->deplibs->pool, arg);
  entry->dlib = dlhandle;

  return NULL;
}

/* Load an ecs shared library */
static const char *load_library (ap_pool *p, ecs_manager *entry, int do_stat, char *prefix)
{
  ap_os_dso_handle_t dlhandle;
  InitFunc init_func;
  CGIMainFunc cgi_main;
  WrapInitFunc wrap_init;
  char *err;
  struct stat s;

  if (do_stat)
  {
    if (stat(entry->libpath, &s) == -1)
    {
      err = ap_psprintf (p, "Failed to stat library file %s: %d", entry->libpath, errno);
      return err;
    }
    entry->mtime = s.st_mtime;
  }

  if (entry->loaded == 1)
  {
    fprintf (stderr, "Warning: attempting to reload %s but it's already loaded\n", entry->libpath);
  }

  /* This does a RTLD_NOW, if we want lazy, we're going to have to do it
   * ourselves */
  if ((dlhandle = ap_os_dso_load(entry->libpath)) == NULL) {
    return ap_os_dso_error();
  }

  if (entry->dlib == dlhandle)
  {
    fprintf (stderr, "Warning: Reload of %s returned same handle\n", entry->libpath);
  }

  if ((init_func = (InitFunc)ap_os_dso_sym(dlhandle, ECSInit))) {
    (*init_func)();
  }
  if (!(wrap_init = (WrapInitFunc)ap_os_dso_sym(dlhandle, WrapInit))) {
    err = ap_psprintf (p, "Failed to find wrap init function %s in shared object: %s", WrapInit, dlerror());
    ap_os_dso_unload(dlhandle);
    return err;
  }
  if (!(cgi_main = (CGIMainFunc)ap_os_dso_sym(dlhandle, CGIMain))) {
    err = ap_psprintf (p, "Failed to find entry function %s in shared object: %s", CGIMain, dlerror());
    ap_os_dso_unload(dlhandle);
    return err;
  }

  /* Um, this may be a problem... */
  ap_register_cleanup (p, dlhandle, slib_cleanup, dummy);

  entry->dlib = dlhandle;
  entry->wrap_init = wrap_init;
  entry->start = cgi_main;
  entry->loaded = 1;

  fprintf (stderr, "%sLoaded library %s [%d]\n", prefix, entry->libpath, dlhandle);

  return NULL;
}

const char *set_pre_lib (cmd_parms *parms, void *dummy, char *arg)
{
  ecs_server_conf *cls = ap_get_module_config (parms->server->module_config,
      &ecs_module);
  ecs_manager *entry;

  entry = (ecs_manager*)ap_push_array(cls->handlers);
  entry->libpath = ap_pstrdup(cls->handlers->pool, arg);

  return load_library (cls->handlers->pool, entry, 1, "Pre");
}

const char *set_fork (cmd_parms *parms, void *dummy, int flag)
{
  ecs_server_conf *cls = ap_get_module_config (parms->server->module_config,
      &ecs_module);

  cls->fork_enabled = (flag ? 1 : 0);

  return NULL;
}

const char *set_reload (cmd_parms *parms, void *dummy, int flag)
{
  ecs_server_conf *cls = ap_get_module_config (parms->server->module_config,
      &ecs_module);

  cls->reload_enabled = (flag ? 1 : 0);

  return NULL;
}

static ecs_manager *findHandler(array_header *a, char *file)
{
  ecs_manager *list = (ecs_manager*)(a->elts);
  int i;

  for (i = 0; i < a->nelts; i++)
  {
    if (!strcmp(list[i].libpath, file))
      return &(list[i]);
  }
  return NULL;
}

static int run_dl_cgi (ecs_server_conf *sconf, request_rec* r, char* argv0)
{
  int ret = 0;
  void* handle;
  int cgi_status;
  int argc;
  char** argv;
  WRAPPER_DATA *wdata;
  ecs_manager *handler;
  const char *err;

  char** envp = e_setup_cgi_env(r);

  /* Find/open library */
  handler = findHandler (sconf->handlers, r->filename);
  if (handler == NULL)
  {
    ecs_manager my_handler;
    my_handler.libpath = ap_pstrdup(sconf->handlers->pool, r->filename);
    err = load_library(sconf->handlers->pool, &my_handler, 1, "");
    if (err != NULL)
    {
      log_reason("Error opening library:", err, r);
      ret = ERROR;
    }
    else
    {
      handler = (ecs_manager*)ap_push_array(sconf->handlers);
      handler->dlib = my_handler.dlib;
      handler->wrap_init = my_handler.wrap_init;
      handler->start = my_handler.start;
      handler->mtime = my_handler.mtime;
      handler->loaded = my_handler.loaded;
      handler->libpath = my_handler.libpath;
    }
  }
  else if (sconf->reload_enabled)
  {
    struct stat s;
    if (stat(handler->libpath, &s) == -1)
    {
      log_reason("Unable to stat file: ", handler->libpath, r);
      ret = ERROR;
    }
    else if (!handler->loaded || (s.st_mtime > handler->mtime))
    {
      if (handler->loaded)
      {
	int x;
	fprintf (stderr, "Unloading %s\n", handler->libpath);
	slib_cleanup(handler->dlib);
	/* Really unload this thing */
	while ((x < 100) && (dlclose(handler->dlib) != -1)) x++;
	if (x == 100) 
	  fprintf (stderr, "dlclose() never returned -1");
	handler->loaded = 0;
      }
      err = load_library(sconf->handlers->pool, handler, 0, "Re");
      if (err != NULL)
      {
	log_reason("Error opening library:", err, r);
	ret = ERROR;
      }
      handler->mtime = s.st_mtime;
    }
  }

  if (!ret) {
    if ((!r->args) || (!r->args[0]) || (ap_ind(r->args,'=') >= 0) ) 
    {
      argc = 1;
      argv = &argv0;
    } else {
      argv = ecs_create_argv(r->pool, NULL,NULL,NULL,argv0,r->args);
      for (argc = 0 ; argv[argc] ; ++argc);
    }
  }

  /*  Yow ... at last we can go ...

      Now, what to do if CGI crashes (aaargh)
      Methinks an atexit ... cleanup perhaps; have to figgerout
      what the atexit needs to invoke ... yuk!

      Or maybe better to catch SIGSEGV and SIGBUS ?
      - we don't want coredumps from someone else's bugs, do we?
      still doesn't guarantee anything very good :-(

      Ugh .. nothing better???
   */
  if (!ret)
  {
    wdata = (WRAPPER_DATA *) ap_pcalloc (r->pool, sizeof (WRAPPER_DATA));
    /* We use malloc here because there is no pool alloc command for
     * realloc... */
    wdata->hbuf.buf = (char *) malloc (sizeof(char) * 1024);
    wdata->hbuf.max = 1024;
    wdata->r = r;

#ifdef ECS_DEBUG
    fprintf (stderr, "wrap_init()\n");
#endif
    handler->wrap_init(wdata, wrap_read, wrap_vprintf, wrap_write, wrap_getenv, wrap_putenv, wrap_iterenv);

#ifdef ECS_DEBUG
    fprintf (stderr, "cgi_main()\n");
#endif
    cgi_status = handler->start(argc,argv,envp);
    if (cgi_status != 0)
    {
      /*log_reason("CGI returned error status", cgi_status, r) ;*/
      ret = ERROR;
    }

    if (wdata->returns != OK)
      ret = wdata->returns;

    free (wdata->hbuf.buf);
  }

  return ret;
}

int run_xcgi (ecs_server_conf *conf, request_rec* r, char* argv0)
{
  int len_read;
  char argsbuffer[HUGE_STRING_LEN];
  int ret = 0;

  ret = run_dl_cgi (conf, r, argv0);

  if (ret == INTERNAL_REDIRECT)
  {
    const char* location = ap_table_get (r->headers_out, "Location");

    /* This redirect needs to be a GET no matter what the original
     * method was.
     */
    r->method = ap_pstrdup(r->pool, "GET");
    r->method_number = M_GET;

    /* We already read the message body (if any), so don't allow
     * the redirected request to think it has one.  We can ignore 
     * Transfer-Encoding, since we used REQUEST_CHUNKED_ERROR.
     */
    ap_table_unset(r->headers_in, "Content-Length");

    ap_internal_redirect_handler (location, r);
    return OK;
  } 

  return ret;
}

int ecs_handler (request_rec* r)
{
  int retval;
  char *argv0;
  int is_included = !strcmp (r->protocol, "INCLUDED");
  void *sconf = r->server->module_config;
  ecs_server_conf *conf =
    (ecs_server_conf *)ap_get_module_config(sconf, &ecs_module);

  ap_error_log2stderr(r->server);
#ifdef ECS_DEBUG
  fprintf(stderr, "running ecs_handler %s\n", r->filename);
#endif

  if((argv0 = strrchr(r->filename,'/')) != NULL)
    argv0++;
  else argv0 = r->filename;

  if (!(ap_allow_options (r) & OPT_EXECCGI) )
    return log_scripterror(r, conf, FORBIDDEN,
	"Options ExecCGI is off in this directory");

  if (S_ISDIR(r->finfo.st_mode))
    return log_scripterror(r, conf, FORBIDDEN,
	"attempt to invoke directory as script");
  if (r->finfo.st_mode == 0)
    return log_scripterror(r, conf, NOT_FOUND,
	"file not found or unable to stat");

#ifdef ECS_DEBUG
  fprintf (stderr, "ap_setup_client_block\n");
#endif
  if ((retval = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR)))
    return retval;

#ifdef ECS_DEBUG
  fprintf (stderr, "before run\n");
#endif
  return run_xcgi(conf, r, argv0);
}

handler_rec ecs_handlers[] = {
  { ECS_MAGIC_TYPE, ecs_handler },
  { "ecs-cgi", ecs_handler},
  { NULL }
};

command_rec ecs_cmds[] = {
 { "ECSFork", set_fork, NULL, OR_FILEINFO, FLAG,
   "On or off to enable or disable (default) forking before calling cgi_main" },
 { "ECSReload", set_reload, NULL, OR_FILEINFO, FLAG,
   "On or off to enable or disable (default) checking if the shared library\n" \
   "  has changed and reloading it if it has"},
 { "ECSDepLib", set_dep_lib, NULL, RSRC_CONF, TAKE1,
   "The location of a dependent lib to dlopen during init"},
 { "ECSPreload", set_pre_lib, NULL, RSRC_CONF, TAKE1,
   "The location of a shared lib handler to preload during init"},
 { NULL }
};

module ecs_module = {
   STANDARD_MODULE_STUFF,
   NULL,			/* initializer */
   NULL,			/* dir config creater */
   NULL,			/* dir merger --- default is to override */
   create_ecs_config,		/* server config */
   NULL, /*merge_ecs_config,*/	       	/* merge server config */
   ecs_cmds,			/* command table */
   ecs_handlers,		/* handlers */
   NULL,			/* filename translation */ 
   NULL,			/* check_user_id */
   NULL,			/* check auth */
   NULL,			/* check access */
   NULL,			/* type_checker */
   NULL,			/* fixups */
   NULL,			/* logger */
#if MODULE_MAGIC_NUMBER >= 19970103
   NULL,       			/* [3] header parser */ 
#endif
#if MODULE_MAGIC_NUMBER >= 19970719
   NULL,       			/* process initializer */
#endif
#if MODULE_MAGIC_NUMBER >= 19970728
   NULL,       			/* process exit/cleanup */
#endif
#if MODULE_MAGIC_NUMBER >= 19970902
   NULL,       			/* [1] post read_request handling */
#endif
};


/* Here's some stuff that essentially duplicates util_script.c
   This really should be merged, but if _I_ do that it'll break
   modularity and leave users with a nasty versioning problem.

   If I get a round tuit sometime, I might ask the Apache folks
   about integrating some changes in the main source tree.
*/
/* If a request includes query info in the URL (stuff after "?"), and
 * the query info does not contain "=" (indicative of a FORM submission),
 * then this routine is called to create the argument list to be passed
 * to the CGI script.  When suexec is enabled, the suexec path, user, and
 * group are the first three arguments to be passed; if not, all three
 * must be NULL.  The query info is split into separate arguments, where
 * "+" is the separator between keyword arguments.
 */
char **ecs_create_argv(pool *p, char *path, char *user, char *group,
                          char *av0, const char *args)
{
    int x, numwords;
    char **av;
    char *w;
    int idx = 0;

    /* count the number of keywords */

    for (x = 0, numwords = 1; args[x]; x++)
        if (args[x] == '+') ++numwords;

    if (numwords > APACHE_ARG_MAX - 5) {
        numwords = APACHE_ARG_MAX - 5; /* Truncate args to prevent overrun */
    }
    av = (char **)ap_palloc(p, (numwords + 5) * sizeof(char *));

    if (path)
        av[idx++] = path;
    if (user)
        av[idx++] = user;
    if (group)
        av[idx++] = group;

    av[idx++] = av0;

    for (x = 1; x <= numwords; x++) {
        w = ap_getword_nulls(p, &args, '+');
        ap_unescape_url(w);
        av[idx++] = ap_escape_shell_cmd(p, w);
    }
    av[idx] = NULL;
    return av;
}
