#!/usr/bin/env ruby

require 'mkmf'

# dir_config("hdf","../../..","../../../libs")
dir_config("hdf")

if have_header("ClearSilver.h") && have_library("neo_utl","hdf_init") && have_library("neo_cs","cs_init")
  create_makefile("hdf")
end
