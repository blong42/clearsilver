/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#include <Python.h>

#include "util/neo_err.h"
#include "util/neo_misc.h"
#include "util/neo_str.h"
#include "util/neo_hdf.h"
#include "cgi/cgi.h"
#include "cgi/cgiwrap.h"
#include "cgi/html.h"
#include "p_neo_util.h"

static PyObject *CGIFinishedException;

#define CGIObjectCheck(a) (!(strcmp((a)->ob_type->tp_name, CGIObjectType.tp_name)))

typedef struct _CGIObject
{
   PyObject_HEAD
   CGI *cgi;
   PyObject *hdf;
} CGIObject;

static PyObject *p_cgi_value_get_attr (CGIObject *self, char *name);
static void p_cgi_dealloc (CGIObject *ho);

static PyTypeObject CGIObjectType =
{
  PyObject_HEAD_INIT(&PyType_Type)
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
  PyMem_DEL(ho);
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
  char *file;

  if (!PyArg_ParseTuple(args, "s:CGI(file)", &file))
    return NULL;

  err = cgi_init (&cgi, file);
  if (err) return p_neo_error (err);
  return p_cgi_to_object (cgi);
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
  NEOERR *err;
  static char *kwlist[] = {"name", "value", "path", "domain", "time_str", "persist", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|sssi:cookieSet()", kwlist, &name, &value, &path, &domain, &time_str, &persist))
    return NULL;

  err = cgi_cookie_set (cgi, name, value, path, domain, time_str, persist);
  if (err) return p_neo_error (err);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject * p_cgi_cookie_clear (PyObject *self, PyObject *args)
{
  CGI *cgi = ((CGIObject *) self)->cgi;
  char *name, *domain = NULL, *path = NULL;
  NEOERR *err;

  if (!PyArg_ParseTuple(args, "s|ss:cookieClear(name, domain, path)", &name, domain, path))
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
  return PyFile_FromFile (fp, "name", "w+", NULL);
}

static PyMethodDef CGIMethods[] =
{
#if 0
  {"debugInit", p_cgi_debug_init, METH_VARARGS, NULL},
  {"wrapInit", p_cgi_wrap_init, METH_VARARGS, NULL},
#endif
  {"error", p_cgi_error, METH_VARARGS, NULL},
  {"display", p_cgi_display, METH_VARARGS, NULL},
  {"redirect", p_cgi_redirect, METH_VARARGS, NULL},
  {"redirectUri", p_cgi_redirect_uri, METH_VARARGS, NULL},
  {"cookieAuthority", p_cgi_cookie_authority, METH_VARARGS, NULL},
  {"cookieSet", (PyCFunction)p_cgi_cookie_set, METH_VARARGS|METH_KEYWORDS, NULL},
  {"cookieClear", p_cgi_cookie_clear, METH_VARARGS, NULL},
  {"filehandle", p_cgi_filehandle, METH_VARARGS, NULL},
  {NULL, NULL}
};

static PyObject * p_cgi_url_escape (PyObject *self, PyObject *args)
{
  char *s, *esc;
  NEOERR *err;
  PyObject *rv;

  if (!PyArg_ParseTuple(args, "s:urlEscape(str)", &s))
    return NULL;

  err = cgi_url_escape (s, &esc);
  if (err) return p_neo_error (err);
  rv = Py_BuildValue ("s", esc);
  free (esc);
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

static PyObject * p_text_html (PyObject *self, PyObject *args)
{
  char *s, *esc;
  NEOERR *err;
  PyObject *rv;
  int len;

  if (!PyArg_ParseTuple(args, "s#:text2html(str)", &s, &len))
    return NULL;

  err = convert_text_html_alloc (s, len, &esc);
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

static int p_writef (void *data, char *fmt, va_list ap)
{
  WRAPPER_DATA *wrap = (WRAPPER_DATA *)data;
  PyObject *str;
  char buf[1024];
  int len;
  int err;


  len = vsnprintf (buf, sizeof(buf), fmt, ap);

  str = PyString_FromStringAndSize (buf, len);

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

static int p_write (void *data, char *buf, int len)
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
    if (n < 0 && result != NULL) {
      int len = PyString_Size(result);
      if (len == 0) {
	Py_DECREF(result);
	result = NULL;
	PyErr_SetString(PyExc_EOFError,
	    "EOF on object.read()");
      }
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
    return 0;
  }

  len = PyString_Size(buf);
  s = PyString_AsString(buf);

  memcpy (ptr, s, len);

  PyErr_Clear();
  return len;
}

static char *p_getenv (void *data, char *s)
{
  WRAPPER_DATA *wrap = (WRAPPER_DATA *)data;
  PyObject *get;
  PyObject *args;
  PyObject *result;
  char *ret = NULL;

  get = PyObject_GetAttrString(wrap->p_env, "__getitem__");
  if (get == NULL)
  {
    PyErr_Clear();
    return NULL;
  }
  args = Py_BuildValue("(s)", s);
  if (args == NULL) {
    Py_DECREF(get);
    PyErr_Clear();
    return NULL;
  }
  result = PyEval_CallObject(get, args);
  Py_DECREF(get);
  Py_DECREF(args);
  if (result != NULL && !PyString_Check(result)) {
    Py_DECREF(result);
    result = NULL;
    PyErr_SetString(PyExc_TypeError,
	"env.get() returned non-string");
  }
  if (result != NULL)
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
    /* ne_warn ("p_iterenv: Unable to get items method"); */
    PyErr_Clear();
    return -1;
  }
  env_list = PyEval_CallObject(items, NULL);
  Py_DECREF(items);
  if (env_list == NULL)
  {
    /* ne_warn ("p_iterenv: Unable to call items method"); */
    PyErr_Clear();
    return -1;
  }
  result = PyList_GetItem (env_list, x);
  if (result == NULL)
  {
    /* ne_warn ("p_iterenv: Unable to get env %d", x); */
    Py_DECREF(env_list); 
    PyErr_Clear();
    return -1;
  }
  k = PyTuple_GetItem (result, 0);
  v = PyTuple_GetItem (result, 1);
  if (k == NULL || v == NULL)
  {
    /* ne_warn ("p_iterenv: Unable to get k,v %s,%s", k, v); */
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

static int p_putenv (void *data, char *k, char *v)
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

static PyMethodDef ModuleMethods[] =
{
  {"CGI", p_cgi_init, METH_VARARGS, NULL},
  {"urlEscape", p_cgi_url_escape, METH_VARARGS, NULL},
  {"htmlEscape", p_html_escape, METH_VARARGS, NULL},
  {"text2html", p_text_html, METH_VARARGS, NULL},
  {"cgiWrap", cgiwrap, METH_VARARGS, cgiwrap_doc},
  {NULL, NULL}
};

void initneo_cgi(void)
{
  PyObject *m, *d;

  initneo_util();
  initneo_cs();

  m = Py_InitModule("neo_cgi", ModuleMethods);
  p_cgiwrap_init (m);
  d = PyModule_GetDict(m);
  CGIFinishedException = PyErr_NewException("neo_cgi.CGIFinished", NULL, NULL);
  PyDict_SetItemString(d, "CGIFinished", CGIFinishedException);
}
