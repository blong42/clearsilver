#!/usr/local/bin/python2.2
#
# Copyright 2001-2004 Brandon Long
# All Rights Reserved.
#
# ClearSilver Templating System
#
# This code is made available under the terms of the ClearSilver License.
# http://www.clearsilver.net/license.hdf
#


import sys, os, getopt
#sys.path.append("../trakken/pysrc")
#sys.path.append("../trakken/pysrc/base")
sys.path.append("../python")
sys.path.append("../python/examples/base")
sys.path.append(".")
import ihooks
import string
from odb import *


class TestDB (Database):
  def __init__ (self, db):
    Database.__init__(self, db)
    self.addTable("test", "test", TestTable)
  
class TestTable (Table):
  def _defineRows(self):
    self.d_addColumn("test_id", kInteger, primarykey = 1, autoincrement = 1)
    self.d_addColumn("test_int", kInteger)
    self.d_addColumn("test_var", kVarString, 255)
    self.d_addColumn("test_fix", kFixedString, 1)
    self.d_addColumn("test_big", kBigString)

class Foo:
  pass

class CDBIGenerator:
  def __init__ (self, db_name, db_class):
    try:
      self.db = db_class(None)
    except TypeError:
      org = Foo()
      org.orgnum =  1
      self.db = db_class(None, org)
    self.dbname = db_name

  def generate(self):
    tables = self.db._tables.keys()
    fp_h = open("cdbi_%s.h" % self.dbname, "w")
    fp_c = open("cdbi_%s.c" % self.dbname, "w")
    fp_h.write("/* AUTOMATICALLY GENERATED FILE -- DO NOT EDIT */\n\n")
    fp_h.write("#ifndef __CDBI_%s_H_\n" % string.upper(self.dbname))
    fp_h.write("#define __CDBI_%s_H_ 1\n" % string.upper(self.dbname))
    fp_h.write("\n#include \"cdbi/cdbi.h\"\n")
    fp_c.write("/* AUTOMATICALLY GENERATED FILE -- DO NOT EDIT */\n\n")
    fp_c.write("#include <stddef.h>\n")
    fp_c.write("#include \"cdbi_%s.h\"\n" % self.dbname)
    for table in tables:
        fp_c.write('\n')
        fp_h.write('\n')
        pub, int = self.generate_table(table, self.db._tables[table])
        fp_c.write(int)
        fp_h.write(pub)
        fp_c.write('\n')
        fp_h.write('\n')
    fp_h.write("\n#endif /* __CDBI_%s_H_ */\n" % string.upper(self.dbname))
    
  def generate_table(self, tblname, tbl):
    cols = tbl.getColumnList()
    out_p = []
    out_i = []
    out_p.append("typedef struct _%s_row {" % tblname)
    out_p.append("  Row_HEAD")
    out_i.append("static CDBI_TABLE_DEF %sDefn[] = {" % tblname)

    for c_name, c_type, c_option in cols:
      offset = "offsetof(%sRow, %s)" % (tblname, c_name)
      if c_option.has_key('primarykey'):
        pk_offset = "offsetof(%sRow, pk_%s)" % (tblname, c_name)
      else:
        pk_offset = "0"
      size = c_option.get('size', 0)
      flags = []
      if c_option.has_key('primarykey'):
        flags.append("DBF_PRIMARY_KEY")
      if c_option.has_key('indexed'):
        flags.append("DBF_INDEXED")
      if c_option.has_key('unique'):
        flags.append("DBF_UNIQUE")
      if c_option.has_key('autoincrement'):
        flags.append("DBF_AUTO_INC")
      if c_option.has_key('notnull'):
        flags.append("DBF_NOT_NULL")
      if c_option.has_key('int_date'):
        flags.append("DBF_TIME_T")
      if c_option.has_key('no_export'):
        flags.append("DBF_NO_EXPORT")

      if c_type in [kInteger, kIncInteger]:
        size = 4
        if c_option.has_key('primarykey'): out_p.append("  int pk_%s;" % c_name)
        out_p.append("  int %s;" % c_name)
      elif c_type in [kVarString, kFixedString]:
        if c_option.has_key('primarykey'): out_p.append("  char pk_%s[%d];" % (c_name, c_option['size'] + 1))
        out_p.append("  char %s[%d];" % (c_name, c_option['size'] + 1))
      elif c_type == kBigString:
        if c_option.has_key('primarykey'): out_p.append("  char *pk_%s;" % c_name)
        out_p.append("  char *%s;" % (c_name))

      flags = string.join(flags, ' | ')
      if not flags: flags = "0"

      out_i.append('  {%s, %d, "%s", %s, %s, %s},' % (c_type, size, c_name, flags, offset, pk_offset))

    out_p.append("} %sRow;" % tblname)
    out_p.append("\nextern CDBI_TABLE %sTable;" % tblname)

    out_i.append("  {0, 0, NULL, 0, 0, 0}")
    out_i.append("};")
    out_i.append("\nCDBI_TABLE %sTable = {\"%s\", %sDefn, sizeof(%sRow)};" % (tblname, tbl.getTableName(), tblname, tblname));
    return string.join(out_p,'\n'), string.join(out_i, '\n')

def test():
  CDBIGenerator("test", TestDB).generate()

def usage(argv0):
  print "%s [--help] <dbname> <db_py_file> <db_class>" % argv0

def main(argv, environ):
  alist, args = getopt.getopt(argv[1:], "", ["help"])

  for (field, val) in alist:
    if field == "--help":
      usage (argv[0])
      return

  dbname = args[0]
  py_file = args[1]
  if py_file[-3:] == ".py":
    py_file = py_file[:-3]
  dbclassname = args[2]

  ml = ihooks.ModuleImporter()
  try:
    mod = ml.import_module(py_file)
  except:
    import handle_error
    sys.stderr.write("Unable to load file %s\n%s" % (py_file, handle_error.exceptionString()))
    sys.exit(-1)

  dbclass = getattr(mod, dbclassname)

  CDBIGenerator(dbname, dbclass).generate()


if __name__ == "__main__":
  main (sys.argv, os.environ)
    
