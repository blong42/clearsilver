#! /usr/bin/env python

"""
usage: %(progname)s [args]
"""


# import startscript; startscript.init(__name__)
import os, sys, string, time, getopt
from log import *

import odb
import MySQLdb

class Database(odb.Database):
  def __init__(self,db, debug=0):
    odb.Database.__init__(self, db, debug=debug)
    self.SQLError = MySQLdb.Error
    
  def escape(self,str):
    if str is None: return None
    return MySQLdb.escape_string(str)

  def listTables(self, cursor=None):
    if cursor is None: cursor = self.defaultCursor()
    cursor.execute("show tables")
    rows = cursor.fetchall()
    tables = []
    for row in rows:
      tables.append(row[0])
    return tables

  def listIndices(self, cursor=None):
    return []

  def listFieldsDict(self, table_name, cursor=None):
    if cursor is None: cursor = self.defaultCursor()
    sql = "show columns from %s" % table_name
    cursor.execute(sql)
    rows = cursor.fetchall()

    columns = {}
    for row in rows:
      colname = row[0]
      columns[colname] = row

    return columns

  def alterTableToMatch(self):
    invalidAppCols, invalidDBCols = self.checkTable()
    if not invalidAppCols: return

    defs = []
    for colname in invalidAppCols.keys():
      col = self.getColumnDef(colname)
      colname = col[0]
      coltype = col[1]
      options = col[2]
      defs.append(self.colTypeToSQLType(colname, coltype, options))

    defs = string.join(defs, ", ")

    sql = "alter table %s add column " % self.getTableName()
    sql = sql + "(" + defs + ")"

    print sql

    cur = self.db.defaultCursor()
    cur.execute(sql)
      

    
