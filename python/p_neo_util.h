/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef __P_NEO_UTIL_H_
#define __P_NEO_UTIL_H_ 1

#include <util/neo_hdf.h>

__BEGIN_DECLS

// * external HDF object interface.

PyObject * p_hdf_to_object (HDF *data, int dealloc);
HDF * p_object_to_hdf (PyObject *ho);

// other functions

PyObject * p_neo_error (NEOERR *err);
PyObject * p_hdf_to_object (HDF *data, int dealloc);
HDF * p_object_to_hdf (PyObject *ho);
void initneo_util(void);
void initneo_cs(void);

__END_DECLS

#endif /* __P_NEO_UTIL_H_ */
