#!/usr/bin/env ruby

require 'mkmf'

# dir_config("hdf","../../..","../../../libs")
dir_config("hdf")

have_library("z", "deflate")

if have_header("ClearSilver.h") && have_library("neo_utl","hdf_init") && have_library("neo_cs","cs_init") && have_library("neo_cgi","cgi_register_strfuncs")
  create_makefile("hdf")
end
