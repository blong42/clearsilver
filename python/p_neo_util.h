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

#ifndef DL_EXPORT
#define DL_EXPORT(x)	x
#endif

__BEGIN_DECLS

/* external HDF object interface. */

#define P_HDF_TO_OBJECT_NUM 0
#define P_HDF_TO_OBJECT_RETURN PyObject *
#define P_HDF_TO_OBJECT_PROTO (HDF *data, int dealloc)

#define P_OBJECT_TO_HDF_NUM 1
#define P_OBJECT_TO_HDF_RETURN HDF *
#define P_OBJECT_TO_HDF_PROTO (PyObject *ho)

#define P_NEO_ERROR_NUM 2
#define P_NEO_ERROR_RETURN PyObject *
#define P_NEO_ERROR_PROTO (NEOERR *err)

/* external CS object interface */
#define P_CS_TO_OBJECT_NUM 3
#define P_CS_TO_OBJECT_RETURN PyObject *
#define P_CS_TO_OBJECT_PROTO (CSPARSE *data)

#define P_NEO_CGI_POINTERS 4

#ifdef NEO_CGI_MODULE
P_HDF_TO_OBJECT_RETURN p_hdf_to_object P_HDF_TO_OBJECT_PROTO;
P_OBJECT_TO_HDF_RETURN p_object_to_hdf P_OBJECT_TO_HDF_PROTO;
P_NEO_ERROR_RETURN p_neo_error P_NEO_ERROR_PROTO;
P_CS_TO_OBJECT_RETURN p_cs_to_object P_CS_TO_OBJECT_PROTO;

/* other functions */

void initneo_util(void);
void initneo_cs(void);

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
    if (PyInt_AsLong(c_api_num_o) < P_NEO_CGI_POINTERS) { \
      PyErr_Format(PyExc_ImportError, "neo_cgi module doesn't match header compiled against, use of this module may cause a core dump: %ld < %ld", PyInt_AsLong(c_api_num_o), (long) P_NEO_CGI_POINTERS); \
    } \
    if (PyCObject_Check(c_api_object)) { \
      NEO_PYTHON_API = (void **)PyCObject_AsVoidPtr(c_api_object); \
    } \
  } \
}

#endif

__END_DECLS

#endif /* __P_NEO_UTIL_H_ */
