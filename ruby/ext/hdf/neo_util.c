/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#include <ruby.h>
#include "ClearSilver.h"

VALUE mNeotonic;
static VALUE cHdf;
VALUE eHdfError;


VALUE r_neo_error (NEOERR *err)
{
  STRING str;
  VALUE errstr;

  string_init (&str);
  if (nerr_match(err, NERR_PARSE)) {
    nerr_error_string (err, &str);
    errstr = rb_str_new2(str.buf);
    string_clear(&str);
    return errstr;
  }
  else {
    nerr_error_traceback (err, &str);
    errstr = rb_str_new2(str.buf);
    string_clear(&str);
    return errstr;
  }
  string_clear (&str);
  return Qnil;
}

static void h_free(void *p) {
  hdf_destroy(p);
}


static VALUE h_init (VALUE self)
{
  return self;
}

VALUE h_new(VALUE class)
{
  HDF *hdf = NULL;
  NEOERR *err;
  VALUE r_hdf;

  err = hdf_init (&hdf);
  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  r_hdf = Data_Wrap_Struct(class, 0, h_free, hdf);
  rb_obj_call_init(r_hdf, 0, NULL);
  return r_hdf;
}

static VALUE h_get_attr (VALUE self, VALUE oName)
{
  HDF *hdf = NULL;
  char *name;
  HDF_ATTR *attr;
  VALUE k,v;
  VALUE rv;

  Data_Get_Struct(self, HDF, hdf);
  name = STR2CSTR(oName);

  rv = rb_hash_new();

  attr = hdf_get_attr(hdf, name);
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
  HDF *hdf= NULL;
  char *name, *key, *value;
  NEOERR *err;

  Data_Get_Struct(self, HDF, hdf);

  name = STR2CSTR(oName);
  key = STR2CSTR(oKey);
  if ( NIL_P(oValue) )
    value = NULL;
  else
    value = STR2CSTR(oValue);

  err = hdf_set_attr(hdf, name, key, value);
  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  return self;
}

static VALUE h_set_value (VALUE self, VALUE oName, VALUE oValue)
{
  HDF *hdf = NULL;
  char *name, *value;
  NEOERR *err;

  Data_Get_Struct(self, HDF, hdf);

  name=STR2CSTR(oName);
  value=STR2CSTR(oValue);

  err = hdf_set_value (hdf, name, value);

  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  return self;
}

static VALUE h_get_int_value (VALUE self, VALUE oName, VALUE oDefault)
{
  HDF *hdf = NULL;
  char *name;
  int r, d = 0;
  VALUE rv;

  Data_Get_Struct(self, HDF, hdf);
  name=STR2CSTR(oName);
  d=NUM2INT(oDefault);

  r = hdf_get_int_value (hdf, name, d);
  rv = INT2NUM(r);
  return rv;
}

static VALUE h_get_value (VALUE self, VALUE oName, VALUE oDefault)
{
  HDF *hdf = NULL;
  char *name;
  char *r, *d = NULL;
  VALUE rv;

  Data_Get_Struct(self, HDF, hdf);
  name=STR2CSTR(oName);
  d=STR2CSTR(oDefault);

  r = hdf_get_value (hdf, name, d);
  rv = rb_str_new2(r);
  return rv;
}

static VALUE h_get_child (VALUE self, VALUE oName)
{
  HDF *hdf = NULL;
  HDF *r;
  VALUE rv;
  char *name;

  Data_Get_Struct(self, HDF, hdf);
  name=STR2CSTR(oName);

  r = hdf_get_child (hdf, name);
  if (r == NULL) {
    return Qnil;
  }

  rv = Data_Wrap_Struct(cHdf, 0, h_free, r);
  return rv;
}


static VALUE h_obj_child (VALUE self)
{
  HDF *hdf = NULL;
  HDF *r = NULL;
  VALUE rv;

  Data_Get_Struct(self, HDF, hdf);
  
  r = hdf_obj_child (hdf);
  if (r == NULL) {
    return Qnil;
  }

  rv = Data_Wrap_Struct(cHdf, 0, h_free, r);
  return rv;
}

static VALUE h_obj_next (VALUE self)
{
  HDF *hdf = NULL;
  HDF *r = NULL;
  VALUE rv;

  Data_Get_Struct(self, HDF, hdf);

  r = hdf_obj_next (hdf);
  if (r == NULL) {
    return Qnil;
  }

  rv = Data_Wrap_Struct(cHdf, 0, h_free, r);
  return rv;
}

static VALUE h_obj_top (VALUE self)
{
  HDF *hdf = NULL;
  HDF *r = NULL;
  VALUE rv;

  Data_Get_Struct(self, HDF, hdf);

  r = hdf_obj_top (hdf);
  if (r == NULL) {
    return Qnil;
  }

  rv = Data_Wrap_Struct(cHdf, 0, h_free, r);
  return rv;
}

static VALUE h_obj_name (VALUE self)
{
  HDF *hdf = NULL;
  VALUE rv;
  char *r;

  Data_Get_Struct(self, HDF, hdf);

  r = hdf_obj_name (hdf);
  if (r == NULL) {
    return Qnil;
  }

  rv = rb_str_new2(r);
  return rv;
}

static VALUE h_obj_attr (VALUE self)
{
  HDF *hdf = NULL;
  HDF_ATTR *attr;
  VALUE k,v;
  VALUE rv;

  Data_Get_Struct(self, HDF, hdf);
  rv = rb_hash_new();
  
  attr = hdf_obj_attr(hdf);
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
  HDF *hdf = NULL;
  VALUE rv;
  char *r;

  Data_Get_Struct(self, HDF, hdf);

  r = hdf_obj_value (hdf);
  if (r == NULL) {
    return Qnil;
  }

  rv = rb_str_new2(r);
  return rv;
}

static VALUE h_read_file (VALUE self, VALUE oPath)
{
  HDF *hdf = NULL;
  char *path;
  NEOERR *err;

  Data_Get_Struct(self, HDF, hdf);

  path=STR2CSTR(oPath);

  err = hdf_read_file (hdf, path);
  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  return self;
}

static VALUE h_write_file (VALUE self, VALUE oPath)
{
  HDF *hdf = NULL;
  char *path;
  NEOERR *err;

  Data_Get_Struct(self, HDF, hdf);

  path=STR2CSTR(oPath);

  err = hdf_write_file (hdf, path);
  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  return self;
}

static VALUE h_write_file_atomic (VALUE self, VALUE oPath)
{
  HDF *hdf = NULL;
  char *path;
  NEOERR *err;

  Data_Get_Struct(self, HDF, hdf);

  path=STR2CSTR(oPath);

  err = hdf_write_file_atomic (hdf, path);
  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  return self;
}

static VALUE h_remove_tree (VALUE self, VALUE oName)
{
  HDF *hdf = NULL;
  char *name;
  NEOERR *err;

  Data_Get_Struct(self, HDF, hdf);
  name = STR2CSTR(oName);

  err = hdf_remove_tree (hdf, name);
  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  return self;
}

static VALUE h_dump (VALUE self)
{
  HDF *hdf = NULL;
  VALUE rv;
  NEOERR *err;
  STRING str;

  string_init (&str);
  
  Data_Get_Struct(self, HDF, hdf);

  err = hdf_dump_str (hdf, NULL, 0, &str);
  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  rv = rb_str_new2(str.buf);
  string_clear (&str);
  return rv;
}

static VALUE h_write_string (VALUE self)
{
  HDF *hdf = NULL;
  VALUE rv;
  NEOERR *err;
  char *s = NULL;

  Data_Get_Struct(self, HDF, hdf);

  err = hdf_write_string (hdf, &s);

  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  rv = rb_str_new2(s);
  if (s) free(s);
  return rv;
}

static VALUE h_read_string (VALUE self, VALUE oString, VALUE oIgnore)
{
  HDF *hdf = NULL;
  NEOERR *err;
  char *s = NULL;
  int ignore = 0;

  Data_Get_Struct(self, HDF, hdf);

  s = STR2CSTR(oString);
  ignore = NUM2INT(oIgnore);

  err = hdf_read_string_ignore (hdf, s, ignore);

  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  return self;
}

static VALUE h_copy (VALUE self, VALUE oName, VALUE oHdfSrc)
{
  HDF *hdf = NULL;
  HDF *src = NULL;
  char *name;
  NEOERR *err;

  Data_Get_Struct(self, HDF, hdf);
  Data_Get_Struct(oHdfSrc, HDF, src);

  name = STR2CSTR(oName);

  if (src == NULL) rb_raise(eHdfError, "second argument must be an Hdf object");

  err = hdf_copy (hdf, name, src);
  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  return self;
}

static VALUE h_set_symlink (VALUE self, VALUE oSrc, VALUE oDest)
{
  HDF *hdf = NULL;
  char *src;
  char *dest;
  NEOERR *err;

  Data_Get_Struct(self, HDF, hdf);
  src = STR2CSTR(oSrc);
  dest = STR2CSTR(oDest);

  err = hdf_set_symlink (hdf, src, dest);
  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  return self;
}

static VALUE h_escape (VALUE self, VALUE oString, VALUE oEsc_char, VALUE oEsc)
{
  VALUE rv;
  char *s;
  char *escape;
  char *esc_char;
  int buflen;
  char *ret = NULL;
  NEOERR *err;

  s = rb_str2cstr(oString,&buflen);
  esc_char = STR2CSTR(oEsc_char);
  escape = STR2CSTR(oEsc);

  err = neos_escape(s, buflen, esc_char[0], escape, &ret);

  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

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
  int buflen;

  s = rb_str2cstr(oString,&buflen);
  esc_char = STR2CSTR(oEsc_char);

  /* This should be changed to use memory from the gc */
  copy = strdup(s);
  if (copy == NULL) rb_raise(rb_eException, "out of memory");

  neos_unescape(copy, buflen, esc_char[0]);

  rv = rb_str_new2(copy);
  free(copy);
  return rv;
}

void Init_cs();

void Init_hdf() {
  mNeotonic = rb_define_module("Neo");
  cHdf = rb_define_class_under(mNeotonic, "Hdf", rb_cObject);

  rb_define_singleton_method(cHdf, "new", h_new, 0);
  rb_define_method(cHdf, "initialize", h_init, 0);
  rb_define_method(cHdf, "get_attr", h_get_attr, 1);
  rb_define_method(cHdf, "set_attr", h_set_attr, 3);
  rb_define_method(cHdf, "set_value", h_set_value, 2);
  rb_define_method(cHdf, "get_int_value", h_get_int_value, 2);
  rb_define_method(cHdf, "get_value", h_get_value, 2);
  rb_define_method(cHdf, "get_child", h_get_child, 1);
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

  eHdfError = rb_define_class_under(mNeotonic, "HdfError", rb_eException);

  Init_Cs();
}
