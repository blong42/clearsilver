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

static VALUE cCs;
extern VALUE mNeotonic;
extern VALUE eHdfError;

VALUE r_neo_error(NEOERR *err);

static void c_free (CSPARSE *csd) {
  if (csd) {
    cs_destroy (&csd);
  }
}

static VALUE c_init (VALUE self) {
  return self;
}

VALUE c_new (VALUE class, VALUE oHdf) {
  CSPARSE *cs = NULL;
  NEOERR *err;
  HDF *hdf;
  VALUE r_cs;

  Data_Get_Struct(oHdf, HDF, hdf);

  if (hdf == NULL) rb_raise(eHdfError, "must include an Hdf object");

  err = cs_init (&cs, hdf);

  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  r_cs = Data_Wrap_Struct(class, 0, c_free, cs);
  rb_obj_call_init(r_cs, 0, NULL);
  return r_cs;
}

static VALUE c_parse_file (VALUE self, VALUE oPath) {
  CSPARSE *cs = NULL;
  NEOERR *err;
  char *path;

  Data_Get_Struct(self, CSPARSE, cs);
  path = STR2CSTR(oPath);

  err = cs_parse_file (cs, path);
  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  return self;
}

static VALUE c_parse_str (VALUE self, VALUE oString)
{
  CSPARSE *cs = NULL;
  NEOERR *err;
  char *s, *ms;
  int l;

  Data_Get_Struct(self, CSPARSE, cs);
  s = rb_str2cstr(oString, &l);

  /* This should be changed to use memory from the gc */
  ms = strdup(s);
  if (ms == NULL) rb_raise(rb_eException, "out of memory");

  err = cs_parse_string (cs, ms, l);
  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  return self;
}

static NEOERR *render_cb (void *ctx, char *buf)
{
  STRING *str= (STRING *)ctx;

  return nerr_pass(string_append(str, buf));
}

static VALUE c_render (VALUE self)
{
  CSPARSE *cs = NULL;
  NEOERR *err;
  STRING str;
  VALUE rv;

  Data_Get_Struct(self, CSPARSE, cs);

  string_init(&str);
  err = cs_render (cs, &str, render_cb);
  if (err) rb_raise(eHdfError, "%s", r_neo_error(err));

  rv = rb_str_new2(str.buf);
  string_clear (&str);
  return rv;
}

void Init_Cs() {
  cCs = rb_define_class_under(mNeotonic, "Cs", rb_cObject);
  rb_define_singleton_method(cCs, "new", c_new, 1);

  rb_define_method(cCs, "initialize", c_init, 0);
  rb_define_method(cCs, "parse_file", c_parse_file, 1);
  rb_define_method(cCs, "parse_string", c_parse_str, 1);
  rb_define_method(cCs, "render", c_render, 0);
}
