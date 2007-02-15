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

#include <ruby.h>
#include <version.h>
#include "ClearSilver.h"
#include "neo_ruby.h"

VALUE mNeotonic;
static VALUE cHdf;
VALUE eHdfError;
static ID id_to_s;

#define Srb_raise(val) rb_raise(eHdfError, "%s/%d %s",__FILE__,__LINE__,RSTRING(val)->ptr)

VALUE r_neo_error (NEOERR *err)
{
  STRING str;
  VALUE errstr;

  string_init (&str);
  nerr_error_string (err, &str);
  errstr = rb_str_new2(str.buf);
  /*
  if (nerr_match(err, NERR_PARSE)) {
  }
  else {
  }
  */
  string_clear (&str);
  return errstr;
}

static void h_free2(t_hdfh *hdfh) {
#ifdef DEBUG
  fprintf(stderr,"freeing hdf 0x%x\n",hdfh);
#endif
  hdf_destroy(&(hdfh->hdf));
  free(hdfh);
}
static void h_free(t_hdfh *hdfh) {
#ifdef DEBUG
  fprintf(stderr,"freeing hdf holder 0x%x of 0x%x\n",hdfh,hdfh->parent);
#endif
  free(hdfh);
}
static void h_mark(t_hdfh *hdfh) {
  /* Only mark the array if this is the top node, only the original node should
     set up the marker.
   */
#ifdef DEBUG
  fprintf(stderr,"marking 0x%x\n",hdfh);
#endif
  if ( ! NIL_P(hdfh->top) )
    rb_gc_mark(hdfh->top);
  else
    fprintf(stderr,"mark top 0x%x\n",hdfh);
}

static VALUE h_init (VALUE self)
{
  return self;
}

VALUE h_new(VALUE class)
{
  t_hdfh *hdfh;
  NEOERR *err;
  VALUE obj;

  obj=Data_Make_Struct(class,t_hdfh,0,h_free2,hdfh);
  err = hdf_init (&(hdfh->hdf));
  if (err) Srb_raise(r_neo_error(err));
#ifdef DEBUG
  fprintf(stderr,"allocated 0x%x\n",(void *)hdfh);
#endif
  hdfh->top=Qnil;
  rb_obj_call_init(obj, 0, NULL);
  return obj;
}

static VALUE h_get_attr (VALUE self, VALUE oName)
{
  t_hdfh *hdfh;
  char *name;
  HDF_ATTR *attr;
  VALUE k,v;
  VALUE rv;

  Data_Get_Struct(self, t_hdfh, hdfh);
  name = STR2CSTR(oName);

  rv = rb_hash_new();

  attr = hdf_get_attr(hdfh->hdf, name);
  while ( attr != NULL ) {
    k=rb_str_new2(attr->key);
    v=rb_str_new2(attr->value);
    rb_hash_aset(rv, k, v);
    attr = attr->next;
  }
  return rv;
}

static VALUE h_set_attr(VALUE self, VALUE oName, VALUE oKey, VALUE oValue)
{
  t_hdfh *hdfh;
  char *name, *key, *value;
  NEOERR *err;

  Data_Get_Struct(self, t_hdfh, hdfh);

  name = STR2CSTR(oName);
  key = STR2CSTR(oKey);
  if ( NIL_P(oValue) )
    value = NULL;
  else
    value = STR2CSTR(oValue);

  err = hdf_set_attr(hdfh->hdf, name, key, value);
  if (err) Srb_raise(r_neo_error(err));

  return self;
}

static VALUE h_set_value (VALUE self, VALUE oName, VALUE oValue)
{
  t_hdfh *hdfh;
  char *name, *value;
  NEOERR *err;

  Data_Get_Struct(self, t_hdfh, hdfh);

  if ( TYPE(oName) == T_STRING )
    name=STR2CSTR(oName);
  else
    name=STR2CSTR(rb_funcall(oName,id_to_s,0));

  if ( TYPE(oValue) == T_STRING )
    value=STR2CSTR(oValue);
  else
    value=STR2CSTR(rb_funcall(oValue,id_to_s,0));

  err = hdf_set_value (hdfh->hdf, name, value);

  if (err) Srb_raise(r_neo_error(err));

  return self;
}

static VALUE h_get_int_value (VALUE self, VALUE oName, VALUE oDefault)
{
  t_hdfh *hdfh;
  char *name;
  int r, d = 0;
  VALUE rv;

  Data_Get_Struct(self, t_hdfh, hdfh);

  name=STR2CSTR(oName);
  d=NUM2INT(oDefault);

  r = hdf_get_int_value (hdfh->hdf, name, d);
  rv = INT2NUM(r);
  return rv;
}

static VALUE h_get_value (VALUE self, VALUE oName, VALUE oDefault)
{
  t_hdfh *hdfh;
  char *name;
  char *r, *d = NULL;
  VALUE rv;

  Data_Get_Struct(self, t_hdfh, hdfh);
  name=STR2CSTR(oName);
  d=STR2CSTR(oDefault);

  r = hdf_get_value (hdfh->hdf, name, d);
  rv = rb_str_new2(r);
  return rv;
}

static VALUE h_get_child (VALUE self, VALUE oName)
{
  t_hdfh *hdfh,*hdfh_new;
  HDF *r;
  VALUE rv;
  char *name;

  Data_Get_Struct(self, t_hdfh, hdfh);
  name=STR2CSTR(oName);

  r = hdf_get_child (hdfh->hdf, name);
  if (r == NULL) {
    return Qnil;
  }
  rv=Data_Make_Struct(cHdf,t_hdfh,h_mark,h_free,hdfh_new);
  hdfh_new->top=self;
  hdfh_new->hdf=r;
  hdfh_new->parent=hdfh;

  return rv;
}

static VALUE h_get_obj (VALUE self, VALUE oName)
{
  t_hdfh *hdfh,*hdfh_new;
  HDF *r;
  VALUE rv;
  char *name;

  Data_Get_Struct(self, t_hdfh, hdfh);
  name=STR2CSTR(oName);

  r = hdf_get_obj (hdfh->hdf, name);
  if (r == NULL) {
    return Qnil;
  }

  rv=Data_Make_Struct(cHdf,t_hdfh,h_mark,h_free,hdfh_new);
  hdfh_new->top=self;
  hdfh_new->hdf=r;
  hdfh_new->parent=hdfh;

  return rv;
}

static VALUE h_get_node (VALUE self, VALUE oName)
{
  t_hdfh *hdfh,*hdfh_new;
  HDF *r;
  VALUE rv;
  char *name;
  NEOERR *err;

  Data_Get_Struct(self, t_hdfh, hdfh);
  name=STR2CSTR(oName);

  err = hdf_get_node (hdfh->hdf, name, &r);
  if (err)
    Srb_raise(r_neo_error(err));

  rv=Data_Make_Struct(cHdf,t_hdfh,h_mark,h_free,hdfh_new);
  hdfh_new->top=self;
  hdfh_new->hdf=r;
  hdfh_new->parent=hdfh;

  return rv;
}


static VALUE h_obj_child (VALUE self)
{
  t_hdfh *hdfh,*hdfh_new;
  HDF *r = NULL;
  VALUE rv;

  Data_Get_Struct(self, t_hdfh, hdfh);
  
  r = hdf_obj_child (hdfh->hdf);
  if (r == NULL) {
    return Qnil;
  }

  rv=Data_Make_Struct(cHdf,t_hdfh,h_mark,h_free,hdfh_new);
  hdfh_new->top=self;
  hdfh_new->hdf=r;
  hdfh_new->parent=hdfh;

  return rv;
}

static VALUE h_obj_next (VALUE self)
{
  t_hdfh *hdfh,*hdfh_new;
  HDF *r = NULL;
  VALUE rv;

  Data_Get_Struct(self, t_hdfh, hdfh);

  r = hdf_obj_next (hdfh->hdf);
  if (r == NULL) {
    return Qnil;
  }

  rv=Data_Make_Struct(cHdf,t_hdfh,h_mark,h_free,hdfh_new);
  hdfh_new->top=self;
  hdfh_new->hdf=r;
  hdfh_new->parent=hdfh;

  return rv;
}

static VALUE h_obj_top (VALUE self)
{
  t_hdfh *hdfh,*hdfh_new;
  HDF *r = NULL;
  VALUE rv;

  Data_Get_Struct(self, t_hdfh, hdfh);

  r = hdf_obj_top (hdfh->hdf);
  if (r == NULL) {
    return Qnil;
  }

  rv=Data_Make_Struct(cHdf,t_hdfh,h_mark,h_free,hdfh_new);
  hdfh_new->top=self;
  hdfh_new->hdf=r;
  hdfh_new->parent=hdfh;

  return rv;
}

static VALUE h_obj_name (VALUE self)
{
  t_hdfh *hdfh;
  VALUE rv;
  char *r;

  Data_Get_Struct(self, t_hdfh, hdfh);

  r = hdf_obj_name (hdfh->hdf);
  if (r == NULL) {
    return Qnil;
  }

  rv = rb_str_new2(r);
  return rv;
}

static VALUE h_obj_attr (VALUE self)
{
  t_hdfh *hdfh;
  HDF_ATTR *attr;
  VALUE k,v;
  VALUE rv;

  Data_Get_Struct(self, t_hdfh, hdfh);
  rv = rb_hash_new();
  
  attr = hdf_obj_attr(hdfh->hdf);
  while ( attr != NULL ) {
    k=rb_str_new2(attr->key);
    v=rb_str_new2(attr->value);
    rb_hash_aset(rv, k, v);
    attr = attr->next;
  }
  return rv;
}


static VALUE h_obj_value (VALUE self)
{
  t_hdfh *hdfh;
  VALUE rv;
  char *r;

  Data_Get_Struct(self, t_hdfh, hdfh);

  r = hdf_obj_value (hdfh->hdf);
  if (r == NULL) {
    return Qnil;
  }

  rv = rb_str_new2(r);
  return rv;
}

static VALUE h_read_file (VALUE self, VALUE oPath)
{
  t_hdfh *hdfh;
  char *path;
  NEOERR *err;

  Data_Get_Struct(self, t_hdfh, hdfh);

  path=STR2CSTR(oPath);

  err = hdf_read_file (hdfh->hdf, path);
  if (err) Srb_raise(r_neo_error(err));

  return self;
}

static VALUE h_write_file (VALUE self, VALUE oPath)
{
  t_hdfh *hdfh;
  char *path;
  NEOERR *err;

  Data_Get_Struct(self, t_hdfh, hdfh);

  path=STR2CSTR(oPath);

  err = hdf_write_file (hdfh->hdf, path);

  if (err) Srb_raise(r_neo_error(err));

  return self;
}

static VALUE h_write_file_atomic (VALUE self, VALUE oPath)
{
  t_hdfh *hdfh;
  char *path;
  NEOERR *err;

  Data_Get_Struct(self, t_hdfh, hdfh);

  path=STR2CSTR(oPath);

  err = hdf_write_file_atomic (hdfh->hdf, path);
  if (err) Srb_raise(r_neo_error(err));

  return self;
}

static VALUE h_remove_tree (VALUE self, VALUE oName)
{
  t_hdfh *hdfh;
  char *name;
  NEOERR *err;

  Data_Get_Struct(self, t_hdfh, hdfh);
  name = STR2CSTR(oName);

  err = hdf_remove_tree (hdfh->hdf, name);
  if (err) Srb_raise(r_neo_error(err));

  return self;
}

static VALUE h_dump (VALUE self)
{
  t_hdfh *hdfh;
  VALUE rv;
  NEOERR *err;
  STRING str;

  string_init (&str);
  
  Data_Get_Struct(self, t_hdfh, hdfh);

  err = hdf_dump_str (hdfh->hdf, NULL, 0, &str);
  if (err) Srb_raise(r_neo_error(err));

  if (str.len==0)
    return Qnil;

  rv = rb_str_new2(str.buf);
  string_clear (&str);
  return rv;
}

static VALUE h_write_string (VALUE self)
{
  t_hdfh *hdfh;
  VALUE rv;
  NEOERR *err;
  char *s = NULL;

  Data_Get_Struct(self, t_hdfh, hdfh);

  err = hdf_write_string (hdfh->hdf, &s);

  if (err) Srb_raise(r_neo_error(err));

  rv = rb_str_new2(s);
  if (s) free(s);
  return rv;
}

static VALUE h_read_string (VALUE self, VALUE oString, VALUE oIgnore)
{
  t_hdfh *hdfh;
  NEOERR *err;
  char *s = NULL;
  int ignore = 0;

  Data_Get_Struct(self, t_hdfh, hdfh);

  s = STR2CSTR(oString);
  ignore = NUM2INT(oIgnore);

  err = hdf_read_string_ignore (hdfh->hdf, s, ignore);

  if (err) Srb_raise(r_neo_error(err));

  return self;
}

static VALUE h_copy (VALUE self, VALUE oName, VALUE oHdfSrc)
{
  t_hdfh *hdfh, *hdfh_src;
  char *name;
  NEOERR *err;

  Data_Get_Struct(self, t_hdfh, hdfh);
  Data_Get_Struct(oHdfSrc, t_hdfh, hdfh_src);

  name = STR2CSTR(oName);

  if (hdfh_src == NULL) rb_raise(eHdfError, "second argument must be an Hdf object");

  err = hdf_copy (hdfh->hdf, name, hdfh_src->hdf);
  if (err) Srb_raise(r_neo_error(err));

  return self;
}

static VALUE h_set_symlink (VALUE self, VALUE oSrc, VALUE oDest)
{
  t_hdfh *hdfh;
  char *src;
  char *dest;
  NEOERR *err;

  Data_Get_Struct(self, t_hdfh, hdfh);
  src = STR2CSTR(oSrc);
  dest = STR2CSTR(oDest);

  err = hdf_set_symlink (hdfh->hdf, src, dest);
  if (err) Srb_raise(r_neo_error(err));

  return self;
}

static VALUE h_escape (VALUE self, VALUE oString, VALUE oEsc_char, VALUE oEsc)
{
  VALUE rv;
  char *s;
  char *escape;
  char *esc_char;
  long buflen;
  char *ret = NULL;
  NEOERR *err;

  s = rb_str2cstr(oString,&buflen);
  esc_char = STR2CSTR(oEsc_char);
  escape = STR2CSTR(oEsc);

  err = neos_escape((UINT8*)s, buflen, esc_char[0], escape, &ret);

  if (err) Srb_raise(r_neo_error(err));

  rv = rb_str_new2(ret);
  free(ret);
  return rv;
}

static VALUE h_unescape (VALUE self, VALUE oString, VALUE oEsc_char)
{
  VALUE rv;
  char *s;
  char *copy;
  char *esc_char;
  long buflen;

  s = rb_str2cstr(oString,&buflen);
  esc_char = STR2CSTR(oEsc_char);

  /* This should be changed to use memory from the gc */
  copy = strdup(s);
  if (copy == NULL) rb_raise(rb_eNoMemError, "out of memory");

  neos_unescape((UINT8*)copy, buflen, esc_char[0]);

  rv = rb_str_new2(copy);
  free(copy);
  return rv;
}

void Init_cs();

void Init_hdf() {

  id_to_s=rb_intern("to_s");

  mNeotonic = rb_define_module("Neo");
  cHdf = rb_define_class_under(mNeotonic, "Hdf", rb_cObject);

  rb_define_singleton_method(cHdf, "new", h_new, 0);
  rb_define_method(cHdf, "initialize", h_init, 0);
  rb_define_method(cHdf, "get_attr", h_get_attr, 1);
  rb_define_method(cHdf, "set_attr", h_set_attr, 3);
  rb_define_method(cHdf, "set_value", h_set_value, 2);
  rb_define_method(cHdf, "put", h_set_value, 2);
  rb_define_method(cHdf, "get_int_value", h_get_int_value, 2);
  rb_define_method(cHdf, "get_value", h_get_value, 2);
  rb_define_method(cHdf, "get_child", h_get_child, 1);
  rb_define_method(cHdf, "get_obj", h_get_obj, 1);
  rb_define_method(cHdf, "get_node", h_get_node, 1);
  rb_define_method(cHdf, "obj_child", h_obj_child, 0);
  rb_define_method(cHdf, "obj_next", h_obj_next, 0);
  rb_define_method(cHdf, "obj_top", h_obj_top, 0);
  rb_define_method(cHdf, "obj_name", h_obj_name, 0);
  rb_define_method(cHdf, "obj_attr", h_obj_attr, 0);
  rb_define_method(cHdf, "obj_value", h_obj_value, 0);
  rb_define_method(cHdf, "read_file", h_read_file, 1);
  rb_define_method(cHdf, "write_file", h_write_file, 1);
  rb_define_method(cHdf, "write_file_atomic", h_write_file_atomic, 1);
  rb_define_method(cHdf, "remove_tree", h_remove_tree, 1);
  rb_define_method(cHdf, "dump", h_dump, 0);
  rb_define_method(cHdf, "write_string", h_write_string, 0);
  rb_define_method(cHdf, "read_string", h_read_string, 2);
  rb_define_method(cHdf, "copy", h_copy, 2);
  rb_define_method(cHdf, "set_symlink", h_set_symlink, 2);

  rb_define_singleton_method(cHdf, "escape", h_escape, 3);
  rb_define_singleton_method(cHdf, "unescape", h_unescape, 3);

  eHdfError = rb_define_class_under(mNeotonic, "HdfError",
#if RUBY_VERSION_MINOR >= 6
				    rb_eStandardError);
#else
                                    rb_eException);
#endif

  Init_cs();
}
