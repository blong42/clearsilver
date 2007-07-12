
#ifndef __NEO_RUBY_H_
#define __NEO_RUBY_H_

typedef struct s_hdfh {
  HDF *hdf;
  struct s_hdfh *parent;
  VALUE top;
} t_hdfh;

#endif
