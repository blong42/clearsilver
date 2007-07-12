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
#include "ClearSilver.h"
#include "neo_ruby.h"

static VALUE cCs;
extern VALUE mNeotonic;
extern VALUE eHdfError;

VALUE r_neo_error(NEOERR *err);

#define Srb_raise(val) rb_raise(eHdfError, "%s/%d %s",__FILE__,__LINE__,RSTRING(val)->ptr)

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
  t_hdfh *hdfh;
  VALUE r_cs;

  Data_Get_Struct(oHdf, t_hdfh, hdfh);

  if (hdfh == NULL) rb_raise(eHdfError, "must include an Hdf object");

  err = cs_init (&cs, hdfh->hdf);
  if (err) Srb_raise(r_neo_error(err));
  err = cgi_register_strfuncs(cs);
  if (err) Srb_raise(r_neo_error(err));

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
  if (err) Srb_raise(r_neo_error(err));

  return self;
}

static VALUE c_parse_str (VALUE self, VALUE oString)
{
  CSPARSE *cs = NULL;
  NEOERR *err;
  char *s, *ms;
  long l;

  Data_Get_Struct(self, CSPARSE, cs);
  s = rb_str2cstr(oString, &l);

  /* This should be changed to use memory from the gc */
  ms = strdup(s);
  if (ms == NULL) rb_raise(rb_eNoMemError, "out of memory");

  err = cs_parse_string (cs, ms, (size_t)l);

  if (err) Srb_raise(r_neo_error(err));

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
  if (err) Srb_raise(r_neo_error(err));

  rv = rb_str_new2(str.buf);
  string_clear (&str);
  return rv;
}

void Init_cs() {
  cCs = rb_define_class_under(mNeotonic, "Cs", rb_cObject);
  rb_define_singleton_method(cCs, "new", c_new, 1);

  rb_define_method(cCs, "initialize", c_init, 0);
  rb_define_method(cCs, "parse_file", c_parse_file, 1);
  rb_define_method(cCs, "parse_string", c_parse_str, 1);
  rb_define_method(cCs, "render", c_render, 0);
}
