
#include <Python.h>
#include "util/neo_err.h"
#include "util/neo_misc.h"
#include "util/neo_str.h"
#include "util/neo_hdf.h"

static PyObject *NeoError;

PyObject * p_neo_error (NEOERR *err)
{
  STRING str;

  string_init (&str);
  nerr_error_string (err, &str);
  PyErr_SetString (NeoError, str.buf);
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

static PyTypeObject HDFObjectType =
{
  PyObject_HEAD_INIT(&PyType_Type)
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
  PyMem_DEL(ho);
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

  if (!PyArg_ParseTuple(args, "ss:setValue(name, value)", &name, &value))
    return NULL;

  err = hdf_set_value (ho->data, name, value);
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

  err = hdf_dump_str (ho->data, NULL, &str);
  if (err) return p_neo_error(err); 
  rv = Py_BuildValue ("s", str.buf);
  string_clear (&str);
  return rv;
}

static PyMethodDef HDFMethods[] =
{
  {"getIntValue", p_hdf_get_int_value, METH_VARARGS, NULL},
  {"getValue", p_hdf_get_value, METH_VARARGS, NULL},
  {"getObj", p_hdf_get_obj, METH_VARARGS, NULL},
  {"getChild", p_hdf_get_child, METH_VARARGS, NULL},
  {"child", p_hdf_obj_child, METH_VARARGS, NULL},
  {"next", p_hdf_obj_next, METH_VARARGS, NULL},
  {"name", p_hdf_obj_name, METH_VARARGS, NULL},
  {"value", p_hdf_obj_value, METH_VARARGS, NULL},
  {"setValue", p_hdf_set_value, METH_VARARGS, NULL},
  {"readFile", p_hdf_read_file, METH_VARARGS, NULL},
  {"writeFile", p_hdf_write_file, METH_VARARGS, NULL},
  {"removeTree", p_hdf_remove_tree, METH_VARARGS, NULL},
  {"dump", p_hdf_dump, METH_VARARGS, NULL},
  {NULL, NULL}
};

static PyMethodDef UtilMethods[] =
{
  {"HDF", p_hdf_init, METH_VARARGS, NULL},
  {NULL, NULL}
};

PyObject *p_hdf_value_get_attr (HDFObject *ho, char *name)
{
  return Py_FindMethod(HDFMethods, (PyObject *)ho, name);
}

void initneo_util(void)
{
  PyObject *m, *d;

  m = Py_InitModule("neo_util", UtilMethods);
  d = PyModule_GetDict(m);
  NeoError = PyErr_NewException("neo_util.error", NULL, NULL);
  PyDict_SetItemString(d, "error", NeoError);
}
