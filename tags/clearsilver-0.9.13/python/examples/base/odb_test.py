#!/usr/bin/env python

from odb import *

## -----------------------------------------------------------------------
##                            T  E  S  T S
## -----------------------------------------------------------------------

import MySQLdb
import odb_mysql
import sqlite
import odb_sqlite
        
def TEST(output=log):
    LOGGING_STATUS[DEV_SELECT] = 1
    LOGGING_STATUS[DEV_UPDATE] = 1

    print "------ TESTING MySQLdb ---------"
    rdb = MySQLdb.connect(host = 'localhost',user='root', passwd = '', db='testdb')
    ndb = MySQLdb.connect(host = 'localhost',user='trakken', passwd = 'trakpas', db='testdb')
    cursor = rdb.cursor()
    
    output("drop table agents")
    try:
        cursor.execute("drop table agents")   # clean out the table
    except:
        pass
    output("creating table")

    SQL = """

    create table agents (
       agent_id integer not null primary key auto_increment,
       login varchar(200) not null,
       unique (login),
       ext_email varchar(200) not null,
       hashed_pw varchar(20) not null,
       name varchar(200),
       auth_level integer default 0,
       ticket_count integer default 0)
       """

    cursor.execute(SQL)
    db = odb_mysql.Database(ndb)
    TEST_DATABASE(rdb,db,output=output)

    print "------ TESTING sqlite ----------"
    rdb = sqlite.connect("/tmp/test.db",autocommit=1)
    cursor = rdb.cursor()
    try:
        cursor.execute("drop table agents")
    except:
        pass
    SQL = """
    create table agents (
       agent_id integer primary key,
       login varchar(200) not null,
       ext_email varchar(200) not null,
       hashed_pw varchar(20),
       name varchar(200),
       auth_level integer default 0,
       ticket_count integer default 0)"""
    cursor.execute(SQL)
    rdb = sqlite.connect("/tmp/test.db",autocommit=1)
    ndb = sqlite.connect("/tmp/test.db",autocommit=1)
        
    db = odb_sqlite.Database(ndb)
    TEST_DATABASE(rdb,db,output=output,is_mysql=0)
    
    


def TEST_DATABASE(rdb,db,output=log,is_mysql=1):

    cursor = rdb.cursor()
    
    class AgentsTable(Table):
        def _defineRows(self):
            self.d_addColumn("agent_id",kInteger,None,primarykey = 1,autoincrement = 1)
            self.d_addColumn("login",kVarString,200,notnull=1)
            self.d_addColumn("ext_email",kVarString,200,notnull=1)
            self.d_addColumn("hashed_pw",kVarString,20,notnull=1)
            self.d_addColumn("name",kBigString,compress_ok=1)
            self.d_addColumn("auth_level",kInteger,None)
            self.d_addColumn("ticket_count",kIncInteger,None)

    tbl = AgentsTable(db,"agents")




    TEST_INSERT_COUNT = 5

    # ---------------------------------------------------------------
    # make sure we can catch a missing row

    try:
        a_row = tbl.fetchRow( ("agent_id", 1000) )
        raise "test error"
    except eNoMatchingRows:
        pass

    output("PASSED! fetch missing row test")

    # --------------------------------------------------------------
    # create new rows and insert them

    for n in range(TEST_INSERT_COUNT):
        new_id = n + 1
        
        newrow = tbl.newRow()
        newrow.name = "name #%d" % new_id
        newrow.login = "name%d" % new_id
        newrow.ext_email = "%d@name" % new_id
        newrow.save()
        if newrow.agent_id != new_id:
            raise "new insert id (%s) does not match expected value (%d)" % (newrow.agent_id,new_id)

    output("PASSED! autoinsert test")

    # --------------------------------------------------------------
    # fetch one row
    a_row = tbl.fetchRow( ("agent_id", 1) )

    if a_row.name != "name #1":
        raise "row data incorrect"

    output("PASSED! fetch one row test")

    # ---------------------------------------------------------------
    # don't change and save it
    # (i.e. the "dummy cursor" string should never be called!)
    #
    try:
        a_row.save(cursor = "dummy cursor")
    except AttributeError, reason:
        raise "row tried to access cursor on save() when no changes were made!"

    output("PASSED! don't save when there are no changed")

    # ---------------------------------------------------------------
    # change, save, load, test
    
    a_row.auth_level = 10
    a_row.save()
    b_row = tbl.fetchRow( ("agent_id", 1) )
    if b_row.auth_level != 10:
        log(repr(b_row))
        raise "save and load failed"
    

    output("PASSED! change, save, load")

    # ---------------------------------------------------------------
    # replace


    repl_row = tbl.newRow(replace=1)
    repl_row.agent_id = a_row.agent_id
    repl_row.login = a_row.login + "-" + a_row.login
    repl_row.ext_email = "foo"
    repl_row.save()

    b_row = tbl.fetchRow( ("agent_id", a_row.agent_id) )
    if b_row.login != repl_row.login:
        raise "replace failed"
    output("PASSED! replace")

    # --------------------------------------------------------------
    # access unknown dict item

    try:
        a = a_row["UNKNOWN_ATTRIBUTE"]
        raise "test error"
    except KeyError, reason:
        pass

    try:
        a_row["UNKNOWN_ATTRIBUTE"] = 1
        raise "test error"
    except KeyError, reason:
        pass

    output("PASSED! unknown dict item exception")

    # --------------------------------------------------------------
    # access unknown attribute
    try:
        a = a_row.UNKNOWN_ATTRIBUTE
        raise "test error"
    except AttributeError, reason:
        pass

    try:
        a_row.UNKNOWN_ATTRIBUTE = 1
        raise "test error"
    except AttributeError, reason:
        pass

    output("PASSED! unknown attribute exception")

 
    # --------------------------------------------------------------
    # use wrong data for column type

    try:
        a_row.agent_id = "this is a string"
        raise "test error"
    except eInvalidData, reason:
        pass

    output("PASSED! invalid data for column type")

    # --------------------------------------------------------------
    # fetch 1 rows

    rows = tbl.fetchRows( ('agent_id', 1) )
    if len(rows) != 1:
        raise "fetchRows() did not return 1 row!" % (TEST_INSERT_COUNT)

    output("PASSED! fetch one row")


    # --------------------------------------------------------------
    # fetch All rows
    
    rows = tbl.fetchAllRows()
    if len(rows) != TEST_INSERT_COUNT:
        for a_row in rows:
            output(repr(a_row))
        raise "fetchAllRows() did not return TEST_INSERT_COUNT(%d) rows!" % (TEST_INSERT_COUNT)

    output("PASSED! fetchall rows")

  
    # --------------------------------------------------------------
    # delete row object

    row = tbl.fetchRow( ('agent_id', 1) )
    row.delete()
    try:
        row = tbl.fetchRow( ('agent_id', 1) )
        raise "delete failed to delete row!"
    except eNoMatchingRows:
        pass

    # --------------------------------------------------------------
    # table deleteRow() call

    row = tbl.fetchRow( ('agent_id',2) )
    tbl.deleteRow( ('agent_id', 2) )
    try:
        row = tbl.fetchRow( ('agent_id',2) )
        raise "table delete failed"
    except eNoMatchingRows:
        pass

    # --------------------------------------------------------------
    # table deleteRow() call

    row = tbl.fetchRow( ('agent_id',3) )
    if row.databaseSizeForColumn('name') != len(row.name):
        raise "databaseSizeForColumn('name') failed"
    
    # --------------------------------------------------------------
    # test inc fields
    row = tbl.newRow()
    new_id = 1092
    row.name = "name #%d" % new_id
    row.login = "name%d" % new_id
    row.ext_email = "%d@name" % new_id
    row.inc('ticket_count')
    row.save()
    new_id = row.agent_id

    trow = tbl.fetchRow( ('agent_id',new_id) )
    if trow.ticket_count != 1:
        raise "ticket_count didn't inc!"

    row.inc('ticket_count', count=2)
    row.save()
    trow = tbl.fetchRow( ('agent_id',new_id) )
    if trow.ticket_count != 3:
        raise "ticket_count wrong, expected 3, got %d" % trow.ticket_count

    trow.inc('ticket_count')
    trow.save()
    if trow.ticket_count != 4:
        raise "ticket_count wrong, expected 4, got %d" % trow.ticket_count

    output("\n==== ALL TESTS PASSED ====")
    

if __name__ == "__main__":
    TEST()

