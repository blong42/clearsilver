#! /usr/bin/env python

"""
usage: %(progname)s [args]
"""


import os, sys, string, time, getopt
from log import *

import odb
import sqlite

import re

# --- these are using for removing nulls from strings
# --- because sqlite can't handle them

def escape_string(str):
    def subfn(m):
        c = m.group(0)
        return "%%%02X" % ord(c)

    return re.sub("('|\0|%)",subfn,str)

def unescape_string(str):
    def subfn(m):
        hexnum = int(m.group(1),16)
        return "%c" % hexnum
    return re.sub("%(..)",subfn,str)

class Database(odb.Database):
  def __init__(self,db, debug=0):
    odb.Database.__init__(self, db, debug=debug)
    self.SQLError = sqlite.Error
    
  def escape(self,str):
    if str is None:
      return None
    elif type(str) == type(""):
      return string.replace(str,"'","''")
    elif type(str) == type(1):
      return str
    else:
      raise "unknown column data type: %s" % type(str)


  def listTables(self, cursor=None):
    if cursor is None: cursor = self.defaultCursor()
    cursor.execute("select name from sqlite_master where type='table'")
    rows = cursor.fetchall()
    tables = []
    for row in rows: tables.append(row[0])
    return tables

  def listIndices(self, cursor=None):
    if cursor is None: cursor = self.defaultCursor()
    cursor.execute("select name from sqlite_master where type='index'")
    rows = cursor.fetchall()
    tables = []
    for row in rows: tables.append(row[0])
    return tables

  def listFieldsDict(self, table_name, cursor=None):
    if cursor is None: cursor = self.defaultCursor()
    sql = "pragma table_info(%s)" % table_name
    cursor.execute(sql)
    rows = cursor.fetchall()

    columns = {}
    for row in rows:
      colname = row[1]
      columns[colname] = row
    return columns

  def _tableCreateStatement(self, table_name, cursor=None):
    if cursor is None: cursor = self.defaultCursor()
    sql = "select sql from sqlite_master where type='table' and name='%s'" % table_name
    print sql
    cursor.execute(sql)
    row = cursor.fetchone()
    sqlstatement = row[0]
    return sqlstatement
    

  def alterTableToMatch(self, table):
    tableName = table.getTableName()
    tmpTableName = tableName + "_" + str(os.getpid())


    invalidAppCols, invalidDBCols = table.checkTable(warnflag=0)

##     if invalidAppCols or invalidDBCols:
##       return

    if not invalidAppCols and not invalidDBCols:
      return


    oldcols = self.listFieldsDict(tableName)
#    tmpcols = oldcols.keys()
    
    tmpcols = []
    newcols = table.getAppColumnList()
    for colname, coltype, options in newcols:
      if oldcols.has_key(colname): tmpcols.append(colname)
    
    tmpcolnames = string.join(tmpcols, ",")
      
    statements = []

    sql = "begin transaction"
    statements.append(sql)

    sql = "create temporary table %s (%s)" % (tmpTableName, tmpcolnames)
    statements.append(sql)

    sql = "insert into %s select %s from %s" % (tmpTableName, tmpcolnames, tableName)
    statements.append(sql)

    sql = "drop table %s" % tableName
    statements.append(sql)
    
    sql = table._createTableSQL()
    statements.append(sql)

    sql = "insert into %s(%s) select %s from %s" % (tableName, tmpcolnames, tmpcolnames, tmpTableName)
    statements.append(sql)

    sql = "drop table %s" % tmpTableName
    statements.append(sql)

    sql = "commit"
    statements.append(sql)

    cur = self.defaultCursor()
    for statement in statements:
#      print statement
      cur.execute(statement)


def test():
  pass

def usage(progname):
  print __doc__ % vars()

def main(argv, stdout, environ):
  progname = argv[0]
  optlist, args = getopt.getopt(argv[1:], "", ["help", "test", "debug"])

  testflag = 0
  if len(args) == 0:
    usage(progname)
    return
  for (field, val) in optlist:
    if field == "--help":
      usage(progname)
      return
    elif field == "--debug":
      debugfull()
    elif field == "--test":
      testflag = 1

  if testflag:
    test()
    return


if __name__ == "__main__":
  main(sys.argv, sys.stdout, os.environ)
