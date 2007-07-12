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

#include <Python.h>
#include "ClearSilver.h"

#define NEO_CGI_MODULE
#include "p_neo_util.h"

static PyObject *CGIFinishedException;

#define CGIObjectCheck(a) (!(strcmp((a)->ob_type->tp_name, CGIObjectType.tp_name)))

typedef struct _CGIObject
{
   PyObject_HEAD
   CGI *cgi;
   PyObject *hdf;
   PyObject *upload_cb;
   PyObject *upload_rock;
   int upload_error;
} CGIObject;

static PyObject *p_cgi_value_get_attr (CGIObject *self, char *name);
static void p_cgi_dealloc (CGIObject *ho);

PyTypeObject CGIObjectType = {
  PyObject_HEAD_INIT(NULL)
    0,			             /*ob_size*/
  "CGIObjectType",	             /*tp_name*/
  sizeof(CGIObject),	     /*tp_size*/
  0,			             /*tp_itemsize*/
  /* methods */
  (destructor)p_cgi_dealloc,	     /*tp_dealloc*/ 
  0,			             /*tp_print*/
  (getattrfunc)p_cgi_value_get_attr,     /*tp_getattr*/
  0,			             /*tp_setattr*/
  0,			             /*tp_compare*/
  (reprfunc)0,                       /*tp_repr*/
  0,                                 /* tp_as_number */
  0,                                 /* tp_as_sequence */
  0,                                 /* tp_as_mapping */
  0,                                 /* tp_as_hash */
};

static void p_cgi_dealloc (CGIObject *ho)
{
  if (ho->cgi)
  {
    cgi_destroy (&(ho->cgi));
  }
  PyObject_DEL(ho);
}

PyObject * p_cgi_to_object (CGI *data)
{
  PyObject *rv;

  if (data == NULL)
  {
    rv = Py_None;
    Py_INCREF (rv);
  }
  else
  {
    CGIObject *ho = PyObject_NEW (CGIObject, &CGIObjectType);
    if (ho == NULL) return NULL;
    ho->cgi = data;
    ho->hdf = p_hdf_to_object (data->hdf, 0);
    Py_INCREF(ho->hdf);
    rv = (PyObject *) ho;
  }
  return rv;
}

static PyObject * p_cgi_init (PyObject *self, PyObject *args)
{
  CGI *cgi = NULL;
  NEOERR *err;

  err = cgi_init (&cgi, NULL);
  if (err) return p_neo_error (err);
  return p_cgi_to_object (cgi);
}

static PyObject * p_cgi_parse (PyObject *self, PyObject *args)
{
  CGI *cgi = ((CGIObject *) self)->cgi;
  CGIObject *p_cgi = (CGIObject *) self;
  PyObject *rv;
  NEOERR *err;

  p_cgi->upload_error = 0;

  err = cgi_parse (cgi);
  if (err) return p_neo_error (err);

  if (p_cgi->upload_error)
  {
    p_cgi->upload_error = 0;
    return NULL;
  }

  rv = Py_None;
  Py_INCREF(rv);
  return rv;
}

static int python_upload_cb (CGI *cgi, int nread, int expected)
{
  CGIObject *self = (CGIObject *)(cgi->data);
  PyObject *cb, *rock;
  PyObject *args, *result;
  int r;

  /* fprintf(stderr, "upload_cb: %d/%d\n", nread, expected); */
  cb = self->upload_cb;
  rock = self->upload_rock;
  
  if (cb == NULL) return 0;
  args = Py_BuildValue("(Oii)", rock, nread, expected);

  if (args == NULL) {
    self->upload_error = 1;
    return 1;
  }
  result = PyEval_CallObject(cb, args);
  Py_DECREF(args);
  if (result != NULL && !PyInt_Check(result)) {
    Py_DECREF(result);
    result = NULL;
    PyErr_SetString(PyExc_TypeError,
	"upload_cb () returned non-integer");
    self->upload_error = 1;
    return 1;
  }
  r = PyInt_AsLong(result);
  Py_DECREF(result);
  result = NULL;
  return r;
}

static PyObject * p_cgi_set_upload_cb (PyObject *self, PyObject *args)
{
  CGI *cgi = ((CGIObject *) self)->cgi;
  CGIObject *p_cgi = (CGIObject *) self;
  PyObject *rock, *cb;

  if (!PyArg_ParseTuple(args, "OO:setUploadCB(rock, func)", &rock, &cb))
    return NULL;

  cgi->data = self;
  cgi->upload_cb = python_upload_cb;
  p_cgi->upload_cb = cb;
  p_cgi->upload_rock = rock;
  p_cgi->upload_error = 0;
  Py_INCREF(cb);
  Py_INCREF(rock);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject * p_cgi_error (PyObject *self, PyObject *args)
{
  CGI *cgi = ((CGIObject *) self)->cgi;
  char *s;
  PyObject *rv;

  if (!PyArg_ParseTuple(args, "s:error(str)", &s))
    return NULL;

  cgi_error (cgi, s);
  rv = Py_None;
  Py_INCREF(rv);
  return rv;
}

static PyObject * p_cgi_display (PyObject *self, PyObject *args)
{
  CGI *cgi = ((CGIObject *) self)->cgi;
  char *file;
  PyObject *rv;
  NEOERR *err;

  if (!PyArg_ParseTuple(args, "s:display(file)", &file))
    return NULL;

  err = cgi_display (cgi, file);
  if (err) return p_neo_error (err);
  rv = Py_None;
  Py_INCREF(rv);
  return rv;
}

static PyObject * p_cgi_redirect (PyObject *self, PyObject *args)
{
  CGI *cgi = ((CGIObject *) self)->cgi;
  char *s;
  PyObject *rv;

  if (!PyArg_ParseTuple(args, "s:redirect(str)", &s))
    return NULL;

  cgi_redirect (cgi, "%s", s);
  rv = Py_None;
  Py_INCREF(rv);
  return rv;
}

static PyObject * p_cgi_redirect_uri (PyObject *self, PyObject *args)
{
  CGI *cgi = ((CGIObject *) self)->cgi;
  char *s;
  PyObject *rv;

  if (!PyArg_ParseTuple(args, "s:redirectUri(str)", &s))
    return NULL;

  cgi_redirect_uri (cgi, "%s", s);
  rv = Py_None;
  Py_INCREF(rv);
  return rv;
}

static PyObject * p_cgi_cookie_authority (PyObject *self, PyObject *args)
{
  CGI *cgi = ((CGIObject *) self)->cgi;
  char *s, *host;
  PyObject *rv;

  if (!PyArg_ParseTuple(args, "s:cookieAuthority(host)", &host))
    return NULL;

  s = cgi_cookie_authority (cgi, host);
  if (s == NULL)
  {
    rv = Py_None;
    Py_INCREF(rv);
  }
  else
  {
    rv = Py_BuildValue ("s", s);
  }
  return rv;
}

static PyObject * p_cgi_cookie_set (PyObject *self, PyObject *args, 
    PyObject *keywds)
{
  CGI *cgi = ((CGIObject *) self)->cgi;
  char *name, *value, *path = NULL, *domain = NULL, *time_str = NULL;
  int persist = 0;
  int secure = 0;
  NEOERR *err;
  static char *kwlist[] = {"name", "value", "path", "domain", "time_str", "persist", "secure", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|sssii:cookieSet()", kwlist, &name, &value, &path, &domain, &time_str, &persist, &secure))
    return NULL;

  err = cgi_cookie_set (cgi, name, value, path, domain, time_str, persist, secure);
  if (err) return p_neo_error (err);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject * p_cgi_cookie_clear (PyObject *self, PyObject *args)
{
  CGI *cgi = ((CGIObject *) self)->cgi;
  char *name, *domain = NULL, *path = NULL;
  NEOERR *err;

  if (!PyArg_ParseTuple(args, "s|ss:cookieClear(name, domain, path)", &name, &domain, &path))
    return NULL;

  err = cgi_cookie_clear (cgi, name, domain, path);
  if (err) return p_neo_error (err);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject * p_cgi_filehandle (PyObject *self, PyObject *args)
{
  CGI *cgi = ((CGIObject *) self)->cgi;
  char *name;
  FILE *fp;

  if (!PyArg_ParseTuple(args, "s:filehandle(form_name)", &name))
    return NULL;

  fp = cgi_filehandle (cgi, name);
  if (fp == NULL)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }
  return PyFile_FromFile (fp, name, "w+", NULL);
}

static PyObject * p_cgi_cs_init (PyObject *self, PyObject *args)
{
  CGI *cgi = ((CGIObject *) self)->cgi;
  NEOERR *err;
  CSPARSE *cs;

  if (!PyArg_ParseTuple(args, ":cs()"))
    return NULL;

  err = cgi_cs_init(cgi, &cs);
  if (err) return p_neo_error (err);
  return p_cs_to_object(cs);
}

static PyMethodDef CGIMethods[] =
{
#if 0
  {"debugInit", p_cgi_debug_init, METH_VARARGS, NULL},
  {"wrapInit", p_cgi_wrap_init, METH_VARARGS, NULL},
#endif
  {"parse", p_cgi_parse, METH_VARARGS, NULL},
  {"setUploadCB", p_cgi_set_upload_cb, METH_VARARGS, NULL},
  {"error", p_cgi_error, METH_VARARGS, NULL},
  {"display", p_cgi_display, METH_VARARGS, NULL},
  {"redirect", p_cgi_redirect, METH_VARARGS, NULL},
  {"redirectUri", p_cgi_redirect_uri, METH_VARARGS, NULL},
  {"cookieAuthority", p_cgi_cookie_authority, METH_VARARGS, NULL},
  {"cookieSet", (PyCFunction)p_cgi_cookie_set, METH_VARARGS|METH_KEYWORDS, NULL},
  {"cookieClear", p_cgi_cookie_clear, METH_VARARGS, NULL},
  {"filehandle", p_cgi_filehandle, METH_VARARGS, NULL},
  {"cs", p_cgi_cs_init, METH_VARARGS, NULL},
  {NULL, NULL}
};

static PyObject * p_cgi_url_escape (PyObject *self, PyObject *args)
{
  char *s, *esc, *o = NULL;
  NEOERR *err;
  PyObject *rv;

  if (!PyArg_ParseTuple(args, "s|s:urlEscape(str, other=None)", &s, &o))
    return NULL;

  err = cgi_url_escape_more (s, &esc, o);
  if (err) return p_neo_error (err);
  rv = Py_BuildValue ("s", esc);
  free (esc);
  return rv;
}

static PyObject * p_cgi_url_unescape (PyObject *self, PyObject *args)
{
  char *s;
  PyObject *rv;
  char *r;

  if (!PyArg_ParseTuple(args, "s:urlUnescape(str)", &s))
    return NULL;

  r = strdup(s);
  if (r == NULL) return PyErr_NoMemory();
  cgi_url_unescape (r);
  rv = Py_BuildValue ("s", r);
  free (r);
  return rv;
}

static PyObject * p_html_escape (PyObject *self, PyObject *args)
{
  char *s, *esc;
  NEOERR *err;
  PyObject *rv;
  int len;

  if (!PyArg_ParseTuple(args, "s#:htmlEscape(str)", &s, &len))
    return NULL;

  err = html_escape_alloc (s, len, &esc);
  if (err) return p_neo_error (err);
  rv = Py_BuildValue ("s", esc);
  free (esc);
  return rv;
}

static PyObject * p_html_strip (PyObject *self, PyObject *args)
{
  char *s, *esc;
  NEOERR *err;
  PyObject *rv;
  int len;

  if (!PyArg_ParseTuple(args, "s#:htmlStrip(str)", &s, &len))
    return NULL;

  err = html_strip_alloc (s, len, &esc);
  if (err) return p_neo_error (err);
  rv = Py_BuildValue ("s", esc);
  free (esc);
  return rv;
}

static PyObject * p_text_html (PyObject *self, PyObject *args, PyObject *keywds)
{
  char *s, *esc;
  NEOERR *err;
  PyObject *rv;
  int len;
  HTML_CONVERT_OPTS opts;
  static char *kwlist[] = {"text", "bounce_url", "url_class", "url_target", "mailto_class", "long_lines", "space_convert", "newlines_convert", "longline_width", "check_ascii_art", "link_name", NULL};

  /* These defaults all come from the old version */
  opts.bounce_url = NULL;
  opts.url_class = NULL;
  opts.url_target = "_blank";
  opts.mailto_class = NULL;
  opts.long_lines = 0;
  opts.space_convert = 0;
  opts.newlines_convert = 1;
  opts.longline_width = 75; /* This hasn't been used in a while, actually */
  opts.check_ascii_art = 1;
  opts.link_name = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#|ssssiiiiis:text2html(text)", 
	kwlist, 
	&s, &len, &(opts.bounce_url), &(opts.url_class), &(opts.url_target), 
	&(opts.mailto_class), &(opts.long_lines), &(opts.space_convert), 
	&(opts.newlines_convert), &(opts.longline_width), &(opts.check_ascii_art), &(opts.link_name)))
    return NULL;

  err = convert_text_html_alloc_options (s, len, &esc, &opts);
  if (err) return p_neo_error (err);
  rv = Py_BuildValue ("s", esc);
  free (esc);
  return rv;
}

PyObject *p_cgi_value_get_attr (CGIObject *ho, char *name)
{
  if (!strcmp(name, "hdf")) 
  {
    Py_INCREF(ho->hdf);
    return ho->hdf;
  }
  return Py_FindMethod(CGIMethods, (PyObject *)ho, name);
}

/* Enable wrapping of newlib stdin/stdout output to go through python */
typedef struct wrapper_data
{
  PyObject *p_stdin;
  PyObject *p_stdout;
  PyObject *p_env;
} WRAPPER_DATA;

static WRAPPER_DATA Wrapper = {NULL, NULL, NULL};

static char cgiwrap_doc[] = "cgiwrap(stdin, stdout, env)\nMethod that will cause all cgiwrapped stdin/stdout functions to be redirected to the python stdin/stdout file objects specified.  Also redirect getenv/putenv calls (env should be either a python dictionary or os.environ)";
static PyObject * cgiwrap (PyObject *self, PyObject *args)
{
  PyObject *p_stdin;
  PyObject *p_stdout;
  PyObject *p_env;

  if (!PyArg_ParseTuple(args, "OOO:cgiwrap(stdin, stdout, env)", &p_stdin, &p_stdout, &p_env))
    return NULL;

  if (p_stdin != Py_None)
  {
    if (Wrapper.p_stdin != NULL)
    {
      Py_DECREF (Wrapper.p_stdin);
    }
    Wrapper.p_stdin = p_stdin;
    Py_INCREF (Wrapper.p_stdin);
  }
  if (p_stdout != Py_None)
  {
    if (Wrapper.p_stdout != NULL)
    {
      Py_DECREF (Wrapper.p_stdout);
    }
    Wrapper.p_stdout = p_stdout;
    Py_INCREF (Wrapper.p_stdout);
  }
  if (p_env != Py_None)
  {
    if (Wrapper.p_env != NULL)
    {
      Py_DECREF (Wrapper.p_env);
    }
    Wrapper.p_env = p_env;
    Py_INCREF (Wrapper.p_env);
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static int p_writef (void *data, const char *fmt, va_list ap)
{
  WRAPPER_DATA *wrap = (WRAPPER_DATA *)data;
  PyObject *str;
  char *buf;
  int len;
  int err;


  buf = vsprintf_alloc(fmt, ap);
  len = visprintf_alloc(&buf, fmt, ap);

  if (buf == NULL)
    return 0;

  str = PyString_FromStringAndSize (buf, len);
  free(buf);

  err = PyFile_WriteObject(str, wrap->p_stdout, Py_PRINT_RAW);
  Py_DECREF(str);

  if (err == 0)
  {
    PyErr_Clear();
    return len;
  }
  PyErr_Clear();
  return err;
}

static int p_write (void *data, const char *buf, int len)
{
  WRAPPER_DATA *wrap = (WRAPPER_DATA *)data;
  PyObject *s;
  int err;

  s = PyString_FromStringAndSize (buf, len);

  err = PyFile_WriteObject(s, wrap->p_stdout, Py_PRINT_RAW);
  Py_DECREF(s);

  if (err == 0)
  {
    PyErr_Clear();
    return len;
  }
  PyErr_Clear();
  return err;
}

/* Similar to the PyFile_GetLine function, this one invokes read on the
 * file object */
static PyObject *PyFile_Read (PyObject *f, int n)
{
  if (f == NULL)
  {
    PyErr_BadInternalCall();
    return NULL;
  }
  /* If this was in the python fileobject code, we could handle this
   * directly for builtin file objects.  Oh well. */
  /* if (!PyFile_Check(f))*/ 
  else
  {
    PyObject *reader;
    PyObject *args;
    PyObject *result;
    reader = PyObject_GetAttrString(f, "read");
    if (reader == NULL)
      return NULL;
    if (n <= 0)
      args = Py_BuildValue("()");
    else
      args = Py_BuildValue("(i)", n);
    if (args == NULL) {
      Py_DECREF(reader);
      return NULL;
    }
    result = PyEval_CallObject(reader, args);
    Py_DECREF(reader);
    Py_DECREF(args);
    if (result != NULL && !PyString_Check(result)) {
      Py_DECREF(result);
      result = NULL;
      PyErr_SetString(PyExc_TypeError,
	  "object.read() returned non-string");
    }
    return result;
  }
}

static int p_read (void *data, char *ptr, int len)
{
  WRAPPER_DATA *wrap = (WRAPPER_DATA *)data;
  PyObject *buf;
  char *s;

  buf = PyFile_Read (wrap->p_stdin, len);

  if (buf == NULL)
  {
    PyErr_Clear();
    return -1;
  }

  len = PyString_Size(buf);
  s = PyString_AsString(buf);

  memcpy (ptr, s, len);

  Py_DECREF(buf);

  PyErr_Clear();
  return len;
}

/* We can't really have an error return from this (and the other
 * cgiwrap) function, because the API doesn't have an error return,
 * and if we get back to python, the error will occur at the next random
 * place that python actually checks for errors independent of an error
 * return.  Not the best way to do things, but its what we've got.  Some
 * of these we can check for in cgiWrap() */
static char *p_getenv (void *data, const char *s)
{
  WRAPPER_DATA *wrap = (WRAPPER_DATA *)data;
  PyObject *get;
  PyObject *args = NULL;
  PyObject *result;
  char *ret = NULL;

  get = PyObject_GetAttrString(wrap->p_env, "__getitem__");
  if (get != NULL)
  {
    args = Py_BuildValue("(s)", s);
    if (args == NULL) {
      Py_DECREF(get);
      PyErr_Clear();
      return NULL;
    }
  }
  else 
  {
    /* Python 1.5.2 and earlier don't have __getitem__ on the standard
     * dict object, so we'll just use get for them */

    get = PyObject_GetAttrString(wrap->p_env, "get");
    if (get != NULL)
    {
      args = Py_BuildValue("(s,O)", s, Py_None);
      if (args == NULL) 
      {
	Py_DECREF(get);
	PyErr_Clear();
	return NULL;
      }
    }
  }
  if (get == NULL)
  {
    ne_warn("Unable to get __getitem__ from env");
    PyErr_Clear();
    return NULL;
  }
  result = PyEval_CallObject(get, args);
  Py_DECREF(get);
  Py_DECREF(args);
  if (result != NULL && !PyString_Check(result) && (result != Py_None)) 
  {
    Py_DECREF(result);
    result = NULL;
    PyErr_SetString(PyExc_TypeError,
	"env.get() returned non-string");
  }
  if (result != NULL && result != Py_None)
  {
    ret = strdup (PyString_AsString(result));
    Py_DECREF (result);
  }

  PyErr_Clear();
  return ret;
}

static int p_iterenv (void *data, int x, char **rk, char **rv)
{
  WRAPPER_DATA *wrap = (WRAPPER_DATA *)data;
  PyObject *items;
  PyObject *env_list;
  PyObject *result;
  PyObject *k, *v;

  items = PyObject_GetAttrString(wrap->p_env, "items");
  if (items == NULL)
  {
    ne_warn ("p_iterenv: Unable to get items method");
    PyErr_Clear();
    return -1;
  }
  env_list = PyEval_CallObject(items, NULL);
  Py_DECREF(items);
  if (env_list == NULL)
  {
    ne_warn ("p_iterenv: Unable to call items method");
    PyErr_Clear();
    return -1;
  }
  if (x >= PyList_Size(env_list))
  {
    *rk = NULL;
    *rv = NULL;
    Py_DECREF(env_list); 
    return 0;
  }
  result = PyList_GetItem (env_list, x);
  if (result == NULL)
  {
    ne_warn ("p_iterenv: Unable to get env %d", x);
    Py_DECREF(env_list); 
    PyErr_Clear();
    return -1;
  }
  k = PyTuple_GetItem (result, 0);
  v = PyTuple_GetItem (result, 1);
  if (k == NULL || v == NULL)
  {
    ne_warn ("p_iterenv: Unable to get k,v %p,%p", k, v); 
    Py_DECREF(env_list); 
    PyErr_Clear();
    return -1;
  }
  *rk = strdup(PyString_AsString(k));
  *rv = strdup(PyString_AsString(v));
  if (*rk == NULL || *rv == NULL)
  {
    if (*rk) free (*rk);
    if (*rv) free (*rv);
    Py_DECREF(env_list); 
    PyErr_Clear();
    return -1;
  }

  Py_DECREF(env_list); 
  PyErr_Clear();
  return 0;
}

static int p_putenv (void *data, const char *k, const char *v)
{
  WRAPPER_DATA *wrap = (WRAPPER_DATA *)data;
  PyObject *set;
  PyObject *args;
  PyObject *result;

  if (k == NULL || v == NULL) return -1;

  set = PyObject_GetAttrString(wrap->p_env, "__setitem__");
  if (set == NULL)
  {
    PyErr_Clear();
    return -1;
  }
  args = Py_BuildValue("(s,s)", k, v);

  if (args == NULL) {
    Py_DECREF(set);
    PyErr_Clear();
    return -1;
  }
  result = PyEval_CallObject(set, args);
  Py_DECREF(set);
  Py_DECREF(args);
  if (result == NULL) 
  {
    PyErr_Clear();
    return -1;
  }
  Py_DECREF(result);
  PyErr_Clear();
  return 0;
}

static void p_cgiwrap_init(PyObject *m)
{
  PyObject *sys, *os, *p_stdin, *p_stdout, *args, *p_env;
#if 0
  PyObject *argv;
  int x;
#endif

  /* Set up the python wrapper 
   * This might not be enough to actually continue to point to
   * sys.stdin/sys.stdout, we'd probably have to actually do the lookup
   * every time... if we need that functionality
   */
  sys = PyImport_ImportModule("sys");
  os = PyImport_ImportModule("os");
  if (sys)
  {
    p_stdin = PyObject_GetAttrString(sys, "stdin");
    p_stdout = PyObject_GetAttrString(sys, "stdout");
#if 0
    argv = PyObject_GetAttrString(sys, "argv");
    if (argv)
    {
      Argc = PyList_Size (argv);
      if (Argc != -1)
      {

	Argv = (char **) malloc (sizeof (char *) * (Argc+1));
	for (x = 0; x < Argc; x++)
	{
	  PyObject *a;
	  char *b;

	  a = PyList_GetItem (argv, x);
	  if (a == NULL)
	    break;
	  b = PyString_AsString(a);
	  if (b == NULL)
	    break;
	  Argv[x] = b;
	}
	Argv[x] = NULL;
      }
    }
#endif
    if (os) 
    {
      p_env = PyObject_GetAttrString(os, "environ");
    }
    else
    {
      Py_INCREF(Py_None);
      p_env = Py_None;
    }
    args = Py_BuildValue("(O,O,O)", p_stdin, p_stdout, p_env);
    if (args)
    {
      cgiwrap_init_emu (&Wrapper, p_read, p_writef, p_write, p_getenv, p_putenv, p_iterenv);
      cgiwrap (m, args);
      Py_DECREF(args);
    }
  }
}

static PyObject * p_ignore (PyObject *self, PyObject *args)
{
  int i = 0;

  if (!PyArg_ParseTuple(args, "i:IgnoreEmptyFormVars(bool)", &i))
    return NULL;

  IgnoreEmptyFormVars = i;
  Py_INCREF(Py_None);
  return Py_None; 
}

static PyObject * p_export_date (PyObject *self, PyObject *args)
{
  NEOERR *err;
  PyObject *ho;
  int i = 0;
  char *prefix;
  char *timezone;
  HDF *hdf;

  if (!PyArg_ParseTuple(args, "Ossi:exportDate(hdf, prefix, timezone, time_t)", &ho, &prefix, &timezone, &i))
    return NULL;

  hdf = p_object_to_hdf (ho);
  if (hdf == NULL)
  {
    PyErr_SetString(PyExc_TypeError, "First argument must be an HDF Object");
    return NULL;
  }

  err = export_date_time_t (hdf, prefix, timezone, i);
  if (err) return p_neo_error (err);

  Py_INCREF(Py_None);
  return Py_None; 
}

static PyObject * p_update (PyObject *self, PyObject *args)
{
  if (_PyImport_FindExtension("neo_util","neo_util") == NULL)
    initneo_util();

  if (_PyImport_FindExtension("neo_cs","neo_cs") == NULL)
    initneo_cs();

  Py_INCREF(Py_None);
  return Py_None; 
}

static PyMethodDef ModuleMethods[] =
{
  {"CGI", p_cgi_init, METH_VARARGS, NULL},
  {"urlEscape", p_cgi_url_escape, METH_VARARGS, NULL},
  {"urlUnescape", p_cgi_url_unescape, METH_VARARGS, NULL},
  {"htmlEscape", p_html_escape, METH_VARARGS, NULL},
  {"htmlStrip", p_html_strip, METH_VARARGS, NULL},
  {"text2html", (PyCFunction)p_text_html, METH_VARARGS|METH_KEYWORDS, NULL},
  {"cgiWrap", cgiwrap, METH_VARARGS, cgiwrap_doc},
  {"IgnoreEmptyFormVars", p_ignore, METH_VARARGS, NULL},
  {"exportDate", p_export_date, METH_VARARGS, NULL},
  {"update", p_update, METH_VARARGS, NULL},
  {NULL, NULL}
};

DL_EXPORT(void) initneo_cgi(void)
{
  PyObject *m, *d;
  static void *NEO_PYTHON_API[P_NEO_CGI_POINTERS];
  PyObject *c_api_object;

  CGIObjectType.ob_type = &PyType_Type;



  initneo_util();
  _PyImport_FixupExtension("neo_util", "neo_util");

  initneo_cs();
  _PyImport_FixupExtension("neo_cs", "neo_cs");

  m = Py_InitModule("neo_cgi", ModuleMethods);
  p_cgiwrap_init (m);
  d = PyModule_GetDict(m);
  CGIFinishedException = PyErr_NewException("neo_cgi.CGIFinished", NULL, NULL);
  PyDict_SetItemString(d, "CGIFinished", CGIFinishedException);

  /* Initialize the C API Pointer array */
  NEO_PYTHON_API[P_HDF_TO_OBJECT_NUM] = (void *)p_hdf_to_object;
  NEO_PYTHON_API[P_OBJECT_TO_HDF_NUM] = (void *)p_object_to_hdf;
  NEO_PYTHON_API[P_NEO_ERROR_NUM] = (void *)p_neo_error;

  /* create a CObject containing the API pointer array's address */
  c_api_object = PyCObject_FromVoidPtr((void *)NEO_PYTHON_API, NULL);
  if (c_api_object != NULL) {
    /* create a name for this object in the module's namespace */
    PyDict_SetItemString(d, "_C_API", c_api_object);
    Py_DECREF(c_api_object);
    PyDict_SetItemString(d, "_C_API_NUM", PyInt_FromLong(P_NEO_CGI_POINTERS));
  }
}
