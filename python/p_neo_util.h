
#ifndef __P_NEO_UTIL_H_
#define __P_NEO_UTIL_H_ 1

PyObject * p_neo_error (NEOERR *err);
PyObject * p_hdf_to_object (HDF *data, int dealloc);
void initneo_util(void);

#endif /* __P_NEO_UTIL_H_ */
