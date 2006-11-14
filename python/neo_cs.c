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


#define CSObjectCheck(a) (!(strcmp((a)->ob_type->tp_name, CSObjectType.tp_name)))

typedef struct _CSObject
{
   PyObject_HEAD
   CSPARSE *data;
} CSObject;

static PyObject *p_cs_value_get_attr (CSObject *self, char *name);
static void p_cs_dealloc (CSObject *ho);

static PyTypeObject CSObjectType =
{
  PyObject_HEAD_INIT(NULL)
    0,			             /*ob_size*/
  "CSObjectType",	             /*tp_name*/
  sizeof(CSObject),	     /*tp_size*/
  0,			             /*tp_itemsize*/
  /* methods */
  (destructor)p_cs_dealloc,	     /*tp_dealloc*/ 
  0,			             /*tp_print*/
  (getattrfunc)p_cs_value_get_attr,     /*tp_getattr*/
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
  /* ne_warn("deallocating hdf: %X", ho); */
  if (ho->data)
  {
    cs_destroy (&(ho->data));
  }
  PyObject_DEL(ho);
}

PyObject * p_cs_to_object (CSPARSE *data)
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
  return p_cs_to_object (cs);
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

  string_init(&str);
  err = cs_render (co->data, &str, render_cb);
  if (err) return p_neo_error(err);
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

PyObject *p_cs_value_get_attr (CSObject *ho, char *name)
{
  return Py_FindMethod(CSMethods, (PyObject *)ho, name);
}

DL_EXPORT(void) initneo_cs(void)
{
  PyObject *m, *d;

  CSObjectType.ob_type = &PyType_Type;

  m = Py_InitModule("neo_cs", ModuleMethods);
  d = PyModule_GetDict(m);
}
