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

typedef struct _CSObject
{
   PyObject_HEAD
   CSPARSE *data;
   PyObject *parent;
} CSObject;

static void p_cs_dealloc (CSObject *ho);

static PyTypeObject CSObjectType =
{
  PyVarObject_HEAD_INIT(NULL, 0)
  "CSObjectType",	             /*tp_name*/
  sizeof(CSObject),	     /*tp_size*/
  0,			             /*tp_itemsize*/
  /* methods */
  (destructor)p_cs_dealloc,	     /*tp_dealloc*/
  0,			             /*tp_print*/
  0,                                 /*tp_getattr*/
  0,			             /*tp_setattr*/
  0,			             /*tp_compare*/
  (reprfunc)0,                       /*tp_repr*/
  0,                                 /* tp_as_number */
  0,                                 /* tp_as_sequence */
  0,                                 /* tp_as_mapping */
  0,                                 /* tp_as_hash */
};

static void p_cs_dealloc (CSObject *ho)
{
  /* ne_warn("p_cs_dealloc(%p), parent = %p, data = %p, hdf = %p\n", ho, ho->parent, ho->data, ho->data->hdf); */
  if (ho->data)
  {
    /* ne_warn("cs_destroy(%p)\n", ho->data); */
    cs_destroy (&(ho->data));
  }
  /* ne_warn("Py_DECREF(%p)\n", ho->parent); */
  Py_DECREF(ho->parent);
  PyObject_DEL(ho);
}

PyObject * p_cs_to_object (CSPARSE *data, PyObject *parent)
{
  PyObject *rv;

  if (data == NULL)
  {
    rv = Py_None;
    Py_INCREF (rv);
  }
  else
  {
    CSObject *ho = PyObject_NEW (CSObject, &CSObjectType);
    if (ho == NULL) return NULL;
    ho->data = data;
    Py_INCREF(parent);
    ho->parent = parent;
    rv = (PyObject *) ho;
    /* ne_warn("allocating cs: %X", ho); */
  }
  return rv;
}

static PyObject * p_cs_init (PyObject *self, PyObject *args)
{
  CSPARSE *cs = NULL;
  NEOERR *err;
  PyObject *ho;
  HDF *hdf;

  if (!PyArg_ParseTuple(args, "O:CS(HDF Object)", &ho))
    return NULL;

  hdf = p_object_to_hdf (ho);
  if (hdf == NULL)
  {
    PyErr_BadArgument();
    return NULL;
  }

  err = cs_init (&cs, hdf);
  if (err) return p_neo_error (err);
  err = cgi_register_strfuncs(cs);
  if (err) return p_neo_error (err);
  return p_cs_to_object (cs, ho);
}

static PyObject * p_cs_parse_file (PyObject *self, PyObject *args)
{
  CSObject *co = (CSObject *)self;
  NEOERR *err;
  char *path;

  if (!PyArg_ParseTuple(args, "s:parseFile(path)", &path))
    return NULL;

  err = cs_parse_file (co->data, path);
  if (err) return p_neo_error(err);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject * p_cs_parse_str (PyObject *self, PyObject *args)
{
  CSObject *co = (CSObject *)self;
  NEOERR *err;
  char *s, *ms;
  int l;

  if (!PyArg_ParseTuple(args, "s#:parseStr(string)", &s, &l))
    return NULL;

  ms = strdup(s);
  if (ms == NULL) return PyErr_NoMemory();

  err = cs_parse_string (co->data, ms, l);
  if (err) return p_neo_error(err);
  Py_INCREF(Py_None);
  return Py_None;
}

static NEOERR *render_cb (void *ctx, char *buf)
{
  STRING *str= (STRING *)ctx;

  return nerr_pass(string_append(str, buf));
}

static PyObject * p_cs_render (PyObject *self, PyObject *args)
{
  CSObject *co = (CSObject *)self;
  NEOERR *err;
  STRING str;
  PyObject *rv;
  int ws_strip_level = 0;
  int do_debug = 0;

  // Copy the Java render, which allows some special options.
  // TODO: perhaps we should pass in whether this is html as well...
  do_debug = hdf_get_int_value (co->data->hdf, "ClearSilver.DisplayDebug", 0);
  ws_strip_level = hdf_get_int_value (co->data->hdf,
                                     "ClearSilver.WhiteSpaceStrip", 0);

  string_init(&str);
  err = cs_render (co->data, &str, render_cb);
  if (err) return p_neo_error(err);

  if (ws_strip_level) {
    cgi_html_ws_strip(&str, ws_strip_level);
  }

  if (do_debug) {
    do {
      err = string_append (&str, "<hr>");
      if (err != STATUS_OK) break;
      err = string_append (&str, "<pre>");
      if (err != STATUS_OK) break;
      err = hdf_remove_tree (co->data->hdf, "Cookie");
      if (err != STATUS_OK) break;
      err = hdf_remove_tree (co->data->hdf, "HTTP.Cookie");
      if (err != STATUS_OK) break;
      err = hdf_dump_str (co->data->hdf, NULL, 0, &str);
      if (err != STATUS_OK) break;
      err = string_append (&str, "</pre>");
      if (err != STATUS_OK) break;
    } while (0);
    if (err) {
      string_clear(&str);
      if (err) return p_neo_error(err);
    }
  }
  rv = Py_BuildValue ("s", str.buf);
  string_clear (&str);
  return rv;
}

static PyMethodDef CSMethods[] =
{
  {"parseFile", p_cs_parse_file, METH_VARARGS, NULL},
  {"parseStr", p_cs_parse_str, METH_VARARGS, NULL},
  {"render", p_cs_render, METH_VARARGS, NULL},
  {NULL, NULL}
};

static PyMethodDef ModuleMethods[] =
{
  {"CS", p_cs_init, METH_VARARGS, NULL},
  {NULL, NULL}
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef ModuleDef = {
  PyModuleDef_HEAD_INIT,
  "neo_cs",
  NULL,  /* module documentation */
  -1,
  ModuleMethods,
};
#endif

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC
PyInit_neo_cs(void)
#else
DL_EXPORT(void) initneo_cs(void)
#endif
{
  PyObject *m;

  CSObjectType.tp_methods = CSMethods;
  if (PyType_Ready(&CSObjectType) < 0)
    return MOD_ERROR_VAL;

#if PY_MAJOR_VERSION >= 3
  m = PyModule_Create(&ModuleDef);
#else
   m = Py_InitModule("neo_cs", ModuleMethods);
#endif
  if (m == NULL)
    return MOD_ERROR_VAL;
  PyModule_GetDict(m);
  return MOD_SUCCESS_VAL(m);
}
