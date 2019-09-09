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

#ifndef __P_NEO_UTIL_H_
#define __P_NEO_UTIL_H_ 1

#include "util/neo_misc.h"
#include "util/neo_hdf.h"
#include "python/capsulethunk.h"

#ifndef DL_EXPORT
#define DL_EXPORT(x)	x
#endif

#ifndef PyVarObject_HEAD_INIT
#define PyVarObject_HEAD_INIT(type, size) \
    PyObject_HEAD_INIT(type) size,
#endif

#ifndef Py_TYPE
#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif

#if PY_MAJOR_VERSION >= 3
#define MOD_FIND_EXTENSION(name) \
    _PyImport_FindExtensionObject(PyUnicode_FromString(name), \
                                  PyUnicode_FromString(name))
#else
#define MOD_FIND_EXTENSION(name) \
    _PyImport_FindExtension(name, name)
#endif

#if PY_MAJOR_VERSION >= 3
#define MOD_ERROR_VAL NULL
#define MOD_SUCCESS_VAL(val) val
#else
#define MOD_ERROR_VAL
#define MOD_SUCCESS_VAL(val)
#endif


__BEGIN_DECLS

/* external HDF object interface. */

#define P_HDF_TO_OBJECT_NUM 0
#define P_HDF_TO_OBJECT_RETURN PyObject *
#define P_HDF_TO_OBJECT_PROTO (HDF *data, PyObject *parent)

#define P_OBJECT_TO_HDF_NUM 1
#define P_OBJECT_TO_HDF_RETURN HDF *
#define P_OBJECT_TO_HDF_PROTO (PyObject *ho)

#define P_NEO_ERROR_NUM 2
#define P_NEO_ERROR_RETURN PyObject *
#define P_NEO_ERROR_PROTO (NEOERR *err)

/* external CS object interface */
#define P_CS_TO_OBJECT_NUM 3
#define P_CS_TO_OBJECT_RETURN PyObject *
#define P_CS_TO_OBJECT_PROTO (CSPARSE *data, PyObject *parent)

#define P_NEO_CGI_POINTERS 4

#ifdef NEO_CGI_MODULE
P_HDF_TO_OBJECT_RETURN p_hdf_to_object P_HDF_TO_OBJECT_PROTO;
P_OBJECT_TO_HDF_RETURN p_object_to_hdf P_OBJECT_TO_HDF_PROTO;
P_NEO_ERROR_RETURN p_neo_error P_NEO_ERROR_PROTO;
P_CS_TO_OBJECT_RETURN p_cs_to_object P_CS_TO_OBJECT_PROTO;

/* other functions */

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_neo_util(void);
PyMODINIT_FUNC PyInit_neo_cs(void);
#else
void initneo_util(void);
void initneo_cs(void);
#endif

#else
static void **NEO_PYTHON_API;

#define p_hdf_to_object \
  (*(P_HDF_TO_OBJECT_RETURN (*)P_HDF_TO_OBJECT_PROTO) NEO_PYTHON_API[P_HDF_TO_OBJECT_NUM])

#define p_object_to_hdf \
  (*(P_OBJECT_TO_HDF_RETURN (*)P_OBJECT_TO_HDF_PROTO) NEO_PYTHON_API[P_OBJECT_TO_HDF_NUM])

#define p_neo_error \
  (*(P_NEO_ERROR_RETURN (*)P_NEO_ERROR_PROTO) NEO_PYTHON_API[P_NEO_ERROR_NUM])

#define p_cs_to_object \
  (*(P_CS_TO_OBJECT_RETURN (*)P_CS_TO_OBJECT_PROTO) NEO_PYTHON_API[P_CS_TO_OBJECT_NUM])

#define import_neo_cgi() \
{ \
  PyObject *module = PyImport_ImportModule("neo_cgi"); \
  if (module != NULL) { \
    PyObject *module_dict = PyModule_GetDict(module); \
    PyObject *c_api_object = PyDict_GetItemString(module_dict, "_C_API"); \
    PyObject *c_api_num_o = PyDict_GetItemString(module_dict, "_C_API_NUM"); \
    if (PyLong_AsLong(c_api_num_o) < P_NEO_CGI_POINTERS) { \
      PyErr_Format(PyExc_ImportError, "neo_cgi module doesn't match header compiled against, use of this module may cause a core dump: %ld < %ld", PyLong_AsLong(c_api_num_o), (long) P_NEO_CGI_POINTERS); \
    } \
    if (PyCapsule_CheckExact(c_api_object)) { \
      NEO_PYTHON_API = (void **)PyCapsule_GetPointer(c_api_object,\
                                                     "NEO_PYTHON_API"); \
    } \
  } \
}

#endif

__END_DECLS

#endif /* __P_NEO_UTIL_H_ */
