
#ifndef __P_NEO_UTIL_H_
#define __P_NEO_UTIL_H_ 1

PyObject * p_neo_error (NEOERR *err);
PyObject * p_hdf_to_object (HDF *data, int dealloc);
HDF * p_object_to_hdf (PyObject *ho);
void initneo_util(void);
void initneo_cs(void);

#endif /* __P_NEO_UTIL_H_ */
