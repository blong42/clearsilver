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

static PyObject *NeoError;
static PyObject *NeoParseError;

PyObject * p_neo_error (NEOERR *err)
{
  STRING str;

  string_init (&str);
  if (nerr_match(err, NERR_PARSE))
  {
    nerr_error_string (err, &str);
    PyErr_SetString (NeoParseError, str.buf);
  }
  else
  {
    nerr_error_traceback (err, &str);
    PyErr_SetString (NeoError, str.buf);
  }
  string_clear (&str);
  return NULL;
}

#define HDFObjectCheck(a) (!(strcmp((a)->ob_type->tp_name, HDFObjectType.tp_name)))

typedef struct _HDFObject
{
   PyObject_HEAD
   HDF *data;
   int dealloc;
} HDFObject;

static PyObject *p_hdf_value_get_attr (HDFObject *self, char *name);
static void p_hdf_dealloc (HDFObject *ho);

static PyTypeObject HDFObjectType = {
  PyObject_HEAD_INIT(NULL)
    0,			             /*ob_size*/
  "HDFObjectType",	             /*tp_name*/
  sizeof(HDFObject),	     /*tp_size*/
  0,			             /*tp_itemsize*/
  /* methods */
  (destructor)p_hdf_dealloc,	     /*tp_dealloc*/ 
  0,			             /*tp_print*/
  (getattrfunc)p_hdf_value_get_attr,     /*tp_getattr*/
  0,			             /*tp_setattr*/
  0,			             /*tp_compare*/
  (reprfunc)0,                       /*tp_repr*/
  0,                                 /* tp_as_number */
  0,                                 /* tp_as_sequence */
  0,                                 /* tp_as_mapping */
  0,                                 /* tp_as_hash */
};


static void p_hdf_dealloc (HDFObject *ho)
{
  /* ne_warn("deallocating hdf: %X", ho); */
  if (ho->data && ho->dealloc)
  {
    hdf_destroy (&(ho->data));
  }
  PyObject_DEL(ho);
}

PyObject * p_hdf_to_object (HDF *data, int dealloc)
{
  PyObject *rv;

  if (data == NULL)
  {
    rv = Py_None;
    Py_INCREF (rv);
  }
  else
  {
    HDFObject *ho = PyObject_NEW (HDFObject, &HDFObjectType);
    if (ho == NULL) return NULL;
    ho->data = data;
    ho->dealloc = dealloc;
    rv = (PyObject *) ho;
    /* ne_warn("allocating hdf: %X", ho); */
  }
  return rv;
}

HDF * p_object_to_hdf (PyObject *ho)
{
  if (HDFObjectCheck(ho))
  {
    return ((HDFObject *)ho)->data;
  }
  return NULL;
}

static PyObject * p_hdf_init (PyObject *self, PyObject *args)
{
  HDF *hdf = NULL;
  NEOERR *err;

  err = hdf_init (&hdf);
  if (err) return p_neo_error (err);
  return p_hdf_to_object (hdf, 1);
}

static PyObject * p_hdf_get_int_value (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *name;
  int r, d = 0;

  if (!PyArg_ParseTuple(args, "si:getIntValue(name, default)", &name, &d))
    return NULL;

  r = hdf_get_int_value (ho->data, name, d);
  rv = Py_BuildValue ("i", r);
  return rv;
}

static PyObject * p_hdf_get_value (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *name;
  char *r, *d = NULL;

  if (!PyArg_ParseTuple(args, "ss:getValue(name, default)", &name, &d))
    return NULL;

  r = hdf_get_value (ho->data, name, d);
  rv = Py_BuildValue ("s", r);
  return rv;
}

static PyObject * p_hdf_get_obj (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *name;
  HDF *r;

  if (!PyArg_ParseTuple(args, "s:getObj(name)", &name))
    return NULL;

  r = hdf_get_obj (ho->data, name);
  if (r == NULL)
  {
    rv = Py_None;
    Py_INCREF(rv);
    return rv;
  }
  rv = p_hdf_to_object (r, 0);
  return rv;
}

static PyObject * p_hdf_get_child (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *name;
  HDF *r;

  if (!PyArg_ParseTuple(args, "s:getChild(name)", &name))
    return NULL;

  r = hdf_get_child (ho->data, name);
  if (r == NULL)
  {
    rv = Py_None;
    Py_INCREF(rv);
    return rv;
  }
  rv = p_hdf_to_object (r, 0);
  return rv;
}

static PyObject * p_hdf_get_attr (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv, *item;
  char *name;
  HDF_ATTR *attr;

  if (!PyArg_ParseTuple(args, "s:getAttrs(name)", &name))
    return NULL;

  rv = PyList_New(0);
  if (rv == NULL) return NULL;
  Py_INCREF(rv);
  attr = hdf_get_attr (ho->data, name);
  while (attr != NULL)
  {
    item = Py_BuildValue("(s,s)", attr->key, attr->value);
    if (item == NULL)
    {
      Py_DECREF(rv); 
      return NULL;
    }
    if (PyList_Append(rv, item) == -1)
    {
      Py_DECREF(rv); 
      return NULL;
    }
    attr = attr->next;
  }
  return rv;
}

static PyObject * p_hdf_obj_attr (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv, *item;
  HDF_ATTR *attr;

  rv = PyList_New(0);
  if (rv == NULL) return NULL;
  Py_INCREF(rv);
  attr = hdf_obj_attr (ho->data);
  while (attr != NULL)
  {
    item = Py_BuildValue("(s,s)", attr->key, attr->value);
    if (item == NULL)
    {
      Py_DECREF(rv); 
      return NULL;
    }
    if (PyList_Append(rv, item) == -1)
    {
      Py_DECREF(rv); 
      return NULL;
    }
    attr = attr->next;
  }
  return rv;
}

static PyObject * p_hdf_obj_child (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  HDF *r;

  r = hdf_obj_child (ho->data);
  if (r == NULL)
  {
    rv = Py_None;
    Py_INCREF(rv);
    return rv;
  }
  rv = p_hdf_to_object (r, 0);
  return rv;
}

static PyObject * p_hdf_obj_next (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  HDF *r;

  r = hdf_obj_next (ho->data);
  if (r == NULL)
  {
    rv = Py_None;
    Py_INCREF(rv);
    return rv;
  }
  rv = p_hdf_to_object (r, 0);
  return rv;
}

static PyObject * p_hdf_obj_top (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  HDF *r;

  r = hdf_obj_top (ho->data);
  if (r == NULL)
  {
    rv = Py_None;
    Py_INCREF(rv);
    return rv;
  }
  rv = p_hdf_to_object (r, 0);
  return rv;
}

static PyObject * p_hdf_obj_name (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *r;

  r = hdf_obj_name (ho->data);
  if (r == NULL)
  {
    rv = Py_None;
    Py_INCREF(rv);
    return rv;
  }
  rv = Py_BuildValue ("s", r);
  return rv;
}

static PyObject * p_hdf_obj_value (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *r;

  r = hdf_obj_value (ho->data);
  if (r == NULL)
  {
    rv = Py_None;
    Py_INCREF(rv);
    return rv;
  }
  rv = Py_BuildValue ("s", r);
  return rv;
}

static PyObject * p_hdf_set_value (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *name, *value;
  NEOERR *err;
  int nlen = 0;
  int vlen = 0;

  if (!PyArg_ParseTuple(args, "s#s#:setValue(name, value)", &name, &nlen, &value, &vlen))
    return NULL;

  err = hdf_set_value (ho->data, name, value);
  if (err) return p_neo_error(err); 

  rv = Py_None;
  Py_INCREF(rv);
  return rv;
}

static PyObject * p_hdf_set_attr (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *name, *value, *key;
  NEOERR *err;

  if (!PyArg_ParseTuple(args, "ssO:setAttr(name, key, value)", &name, &key, &rv))
    return NULL;

  if (PyString_Check(rv))
  {
    value = PyString_AsString(rv);
  } 
  else if (rv == Py_None)
  {
    value = NULL;
  }
  else
  {
    return PyErr_Format(PyExc_TypeError, "Invalid type for value, expected None or string");
  }
  err = hdf_set_attr (ho->data, name, key, value);
  if (err) return p_neo_error(err); 

  rv = Py_None;
  Py_INCREF(rv);
  return rv;
}

static PyObject * p_hdf_read_file (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *path;
  NEOERR *err;

  if (!PyArg_ParseTuple(args, "s:readFile(path)", &path))
    return NULL;

  err = hdf_read_file (ho->data, path);
  if (err) return p_neo_error(err); 

  rv = Py_None;
  Py_INCREF(rv);
  return rv;
}

static PyObject * p_hdf_write_file (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *path;
  NEOERR *err;

  if (!PyArg_ParseTuple(args, "s:writeFile(path)", &path))
    return NULL;

  err = hdf_write_file (ho->data, path);
  if (err) return p_neo_error(err); 

  rv = Py_None;
  Py_INCREF(rv);
  return rv;
}

static PyObject * p_hdf_write_file_atomic (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *path;
  NEOERR *err;

  if (!PyArg_ParseTuple(args, "s:writeFile(path)", &path))
    return NULL;

  err = hdf_write_file_atomic (ho->data, path);
  if (err) return p_neo_error(err); 

  rv = Py_None;
  Py_INCREF(rv);
  return rv;
}

static PyObject * p_hdf_remove_tree (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *name;
  NEOERR *err;

  if (!PyArg_ParseTuple(args, "s:removeTree(name)", &name))
    return NULL;

  err = hdf_remove_tree (ho->data, name);
  if (err) return p_neo_error(err); 

  rv = Py_None;
  Py_INCREF(rv);
  return rv;
}

static PyObject * p_hdf_dump (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  NEOERR *err;
  STRING str;

  string_init (&str);

  err = hdf_dump_str (ho->data, NULL, 0, &str);
  if (err) return p_neo_error(err); 
  rv = Py_BuildValue ("s", str.buf);
  string_clear (&str);
  return rv;
}

static PyObject * p_hdf_write_string (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  NEOERR *err;
  char *s = NULL;

  err = hdf_write_string (ho->data, &s);
  if (err) return p_neo_error(err); 
  rv = Py_BuildValue ("s", s);
  if (s) free(s);
  return rv;
}

static PyObject * p_hdf_read_string (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  NEOERR *err;
  char *s = NULL;
  int ignore = 0;

  if (!PyArg_ParseTuple(args, "s|i:readString(string)", &s, &ignore))
    return NULL;

  err = hdf_read_string_ignore (ho->data, s, ignore);
  if (err) return p_neo_error(err); 
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject * p_hdf_copy (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  HDF *src = NULL;
  PyObject *rv, *o = NULL;
  char *name;
  NEOERR *err;

  if (!PyArg_ParseTuple(args, "sO:copy(name, src_hdf)", &name, &o))
    return NULL;

  src = p_object_to_hdf (o);
  if (src == NULL)
  {
    PyErr_Format(PyExc_TypeError, "second argument must be an HDFObject");
    return NULL;
  }

  err = hdf_copy (ho->data, name, src);
  if (err) return p_neo_error(err); 

  rv = Py_None;
  Py_INCREF(rv);
  return rv;
}

static PyObject * p_hdf_set_symlink (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *src;
  char *dest;
  NEOERR *err;

  if (!PyArg_ParseTuple(args, "ss:setSymLink(src, dest)", &src, &dest))
    return NULL;

  err = hdf_set_symlink (ho->data, src, dest);
  if (err) return p_neo_error(err); 

  rv = Py_None;
  Py_INCREF(rv);
  return rv;
}

static PyObject * p_hdf_search_path (PyObject *self, PyObject *args)
{
  HDFObject *ho = (HDFObject *)self;
  PyObject *rv;
  char *path;
  char full[_POSIX_PATH_MAX];
  NEOERR *err;

  if (!PyArg_ParseTuple(args, "s:searchPath(path)", &path))
    return NULL;

  err = hdf_search_path (ho->data, path, full);
  if (err) return p_neo_error(err); 

  rv = PyString_FromString(full);
  return rv;
}

static PyMethodDef HDFMethods[] =
{
  {"getIntValue", p_hdf_get_int_value, METH_VARARGS, NULL},
  {"getValue", p_hdf_get_value, METH_VARARGS, NULL},
  {"getObj", p_hdf_get_obj, METH_VARARGS, NULL},
  {"getChild", p_hdf_get_child, METH_VARARGS, NULL},
  {"getAttrs", p_hdf_get_attr, METH_VARARGS, NULL},
  {"child", p_hdf_obj_child, METH_VARARGS, NULL},
  {"next", p_hdf_obj_next, METH_VARARGS, NULL},
  {"name", p_hdf_obj_name, METH_VARARGS, NULL},
  {"value", p_hdf_obj_value, METH_VARARGS, NULL},
  {"top", p_hdf_obj_top, METH_VARARGS, NULL},
  {"attrs", p_hdf_obj_attr, METH_VARARGS, NULL},
  {"setValue", p_hdf_set_value, METH_VARARGS, NULL},
  {"setAttr", p_hdf_set_attr, METH_VARARGS, NULL},
  {"readFile", p_hdf_read_file, METH_VARARGS, NULL},
  {"writeFile", p_hdf_write_file, METH_VARARGS, NULL},
  {"writeFileAtomic", p_hdf_write_file_atomic, METH_VARARGS, NULL},
  {"readString", p_hdf_read_string, METH_VARARGS, NULL},
  {"writeString", p_hdf_write_string, METH_VARARGS, NULL},
  {"removeTree", p_hdf_remove_tree, METH_VARARGS, NULL},
  {"dump", p_hdf_dump, METH_VARARGS, NULL},
  {"copy", p_hdf_copy, METH_VARARGS, NULL},
  {"setSymLink", p_hdf_set_symlink, METH_VARARGS, NULL},
  {"searchPath", p_hdf_search_path, METH_VARARGS, NULL},
  {NULL, NULL}
};

static PyObject * p_escape (PyObject *self, PyObject *args)
{
  PyObject *rv;
  char *s;
  char *escape;
  char *esc_char;
  int buflen;
  char *ret = NULL;
  NEOERR *err;

  if (!PyArg_ParseTuple(args, "s#ss:escape(str, char, escape)", &s, &buflen, &esc_char, &escape))
    return NULL;

  err = neos_escape(s, buflen, esc_char[0], escape, &ret);
  if (err) return p_neo_error(err); 

  rv = Py_BuildValue("s", ret);
  free(ret);
  return rv;
}

static PyObject * p_unescape (PyObject *self, PyObject *args)
{
  PyObject *rv;
  char *s;
  char *copy;
  char *esc_char;
  int buflen;

  if (!PyArg_ParseTuple(args, "s#s:unescape(str, char)", &s, &buflen, &esc_char))
    return NULL;

  copy = strdup(s);
  if (copy == NULL) return PyErr_NoMemory();
  neos_unescape(copy, buflen, esc_char[0]);

  rv = Py_BuildValue("s", copy);
  free(copy);
  return rv;
}

/* This returns the expanded version in the standard python time tuple
 * */
static PyObject * p_time_expand (PyObject *self, PyObject *args)
{
  PyObject *rv;
  int tt;
  struct tm ttm;
  char *tz;

  if (!PyArg_ParseTuple(args, "is:time_expand(time_t, timezone string)", &tt, &tz))
    return NULL;

  neo_time_expand(tt, tz, &ttm); 

  rv = Py_BuildValue("(i,i,i,i,i,i,i,i,i)", ttm.tm_year + 1900, ttm.tm_mon + 1, 
      ttm.tm_mday, ttm.tm_hour, ttm.tm_min, ttm.tm_sec, ttm.tm_wday, 0, ttm.tm_isdst);
  return rv;
}

static PyObject * p_time_compact (PyObject *self, PyObject *args)
{
  PyObject *rv;
  int tt;
  struct tm ttm;
  int junk;
  char *tz;

  memset(&ttm, 0, sizeof(struct tm));

  if (!PyArg_ParseTuple(args, "(i,i,i,i,i,i,i,i,i)s:time_compact(time tuple, timezone string)", &ttm.tm_year, &ttm.tm_mon, &ttm.tm_mday, &ttm.tm_hour, &ttm.tm_min, &ttm.tm_sec, &ttm.tm_wday, &junk, &ttm.tm_isdst, &tz))
    return NULL;

  /* fix up difference between ttm and python tup */
  ttm.tm_year -= 1900;
  ttm.tm_mon -= 1;

  tt = neo_time_compact (&ttm, tz);

  rv = Py_BuildValue("i", tt);
  return rv;
}

static PyMethodDef UtilMethods[] =
{
  {"HDF", p_hdf_init, METH_VARARGS, NULL},
  {"escape", p_escape, METH_VARARGS, NULL},
  {"unescape", p_unescape, METH_VARARGS, NULL},
  {"time_expand", p_time_expand, METH_VARARGS, NULL},
  {"time_compact", p_time_compact, METH_VARARGS, NULL},
  {NULL, NULL}
};

PyObject *p_hdf_value_get_attr (HDFObject *ho, char *name)
{
  return Py_FindMethod(HDFMethods, (PyObject *)ho, name);
}

DL_EXPORT(void) initneo_util(void)
{
  PyObject *m, *d;

  HDFObjectType.ob_type = &PyType_Type;

  m = Py_InitModule("neo_util", UtilMethods);
  d = PyModule_GetDict(m);
  NeoError = PyErr_NewException("neo_util.Error", NULL, NULL);
  NeoParseError = PyErr_NewException("neo_util.ParseError", NULL, NULL);
  PyDict_SetItemString(d, "Error", NeoError);
  PyDict_SetItemString(d, "ParseError", NeoParseError);
}
