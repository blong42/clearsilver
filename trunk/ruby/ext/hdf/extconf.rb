#!/usr/bin/env ruby

require 'mkmf'

dir_config("hdf","../../..","../../../libs")

if have_header("util/neo_hdf.h") && have_library("neo_utl","hdf_init") && have_library("neo_cs","cs_init")
  create_makefile("hdf")
end
