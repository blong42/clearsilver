#!/neo/opt/bin/python
#
# odb.py
#
# Object Database Api
#
# Written by David Jeske <jeske@neotonic.com>, 2001/07. 
# Inspired by eGroups' sqldb.py originally written by Scott Hassan circa 1998.
#
# Copyright (C) 2001, by David Jeske and Neotonic
#
# Goals:
#       - a simple object-like interface to database data
#       - database independent (someday)
#       - relational-style "rigid schema definition"
#       - object style easy-access
#
# Example:
#
#  import odb
#  import MySQLdb
#
#  # define table
#  class AgentsTable(odb.Table):
#    def _defineRows(self):
#      self.d_addColumn("agent_id",kInteger,None,primarykey = 1,autoincrement = 1)
#      self.d_addColumn("login",kVarString,200,notnull=1)
#      self.d_addColumn("ticket_count",kIncInteger,None)
#
#  if __name__ == "__main__":
#    # open database
#    ndb = MySQLdb.connect(host = 'localhost',
#                          user='username', 
#                          passwd = 'password', 
#                          db='testdb')
#    db = Database(ndb)
#    tbl = AgentsTable(db,"agents")
#
#    # create row
#    agent_row = tbl.newRow()
#    agent_row.login = "foo"
#    agent_row.save()
#
#    # fetch row (must use primary key)
#    try:
#      get_row = tbl.fetchRow( ('agent_id', agent_row.agent_id) )
#    except odb.eNoMatchingRows:
#      print "this is bad, we should have found the row"
#
#    # fetch rows (can return empty list)
#    list_rows = tbl.fetchRows( ('login', "foo") )
#

import tstart

import string
import sys, zlib
from log import *

import MySQLdb

eNoSuchColumn         = "odb.eNoSuchColumn"
eNonUniqueMatchSpec   = "odb.eNonUniqueMatchSpec"
eNoMatchingRows       = "odb.eNoMatchingRows"
eInternalError        = "odb.eInternalError"
eInvalidMatchSpec     = "odb.eInvalidMatchSpec"
eInvalidData          = "odb.eInvalidData"
eUnsavedObjectLost    = "odb.eUnsavedObjectLost"
eDuplicateKey         = "odb.eDuplicateKey"

#####################################
# COLUMN TYPES                       
################                     ######################
# typename     ####################### size data means:
#              #                     # 
kInteger       = "kInteger"          # -
kFixedString   = "kFixedString"      # size
kVarString     = "kVarString"        # maxsize
kBigString     = "kBigString"        # -
kIncInteger    = "kIncInteger"       # -
kDateTime      = "kDateTime"
kTimeStamp     = "kTimeStamp"


DEBUG = 0

##############
# Database
#
# this will ultimately turn into a mostly abstract base class for
# the DB adaptors for different database types....
#

class Database:
    def __init__(self,db):
        self._tables = {}
        self.db = db
        self._cursor = None
        self.compression_enabled = 0

    # __init__ = None
    # list_tables = None   # list_tables() -> ['a_table','b_table']
    # list_fields = None   # list_fields(tbl_name) -> ['
    # checkTable = None    # checkTable
    # createTable

    def defaultCursor(self):
        if self._cursor is None:
            self._cursor = self.db.cursor()
        return self._cursor

    def escape(self,str):
        return MySQLdb.escape_string(str)
    def defaultRowClass(self):
	return Row

    def defaultRowListClass(self):
        # base type is list...
	return list

    def addTable(self, attrname, tblname, tblclass, rowClass = None, check = 0, create = 0, rowListClass = None):
        self._tables[attrname] = tblclass(self, tblname, rowClass=rowClass, check=check, create=create, rowListClass=rowListClass)

    def close(self):
        for name, tbl in self._tables.items():
            tbl.db = None
        self._tables = {}
        if self.db is not None:
            self.db.close()
            self.db = None

    def __getattr__(self, key):
        if key == "_tables":
            raise AttributeError, "odb.Database: not initialized properly, self._tables does not exist"

        try:
            table_dict = getattr(self,"_tables")
            return table_dict[key]
        except KeyError:
            raise AttributeError, "odb.Database: unknown attribute %s" % (key)
        
    def beginTransaction(self, cursor=None):
        if cursor is None:
            cursor = self.defaultCursor()
        dlog(DEV_UPDATE,"begin")
        cursor.execute("begin")

    def commitTransaction(self, cursor=None):
        if cursor is None:
            cursor = self.defaultCursor()
        dlog(DEV_UPDATE,"commit")
        cursor.execute("commit")

    def rollbackTransaction(self, cursor=None):
        if cursor is None:
            cursor = self.defaultCursor()
        dlog(DEV_UPDATE,"rollback")
        cursor.execute("rollback")

##########################################
# Table
#


class Table:
    def subclassinit(self):
        pass
    def __init__(self,database,table_name,rowClass = None, check = 0, create = 0, rowListClass = None):
	self.db = database
	self.__table_name = table_name
	if rowClass:
	    self.__defaultRowClass = rowClass
	else:
	    self.__defaultRowClass = database.defaultRowClass()

	if rowListClass:
	    self.__defaultRowListClass = rowListClass
	else:
	    self.__defaultRowListClass = database.defaultRowListClass()

	# get this stuff ready!
	
	self.__column_list = []
	self.__vcolumn_list = []
	self.__columns_locked = 0
	self.__has_value_column = 0

	# this will be used during init...
	self.__col_def_hash = None
	self.__vcol_def_hash = None
	self.__primary_key_list = None
        self.__relations_by_table = {}

	# ask the subclass to def his rows
	self._defineRows()

	# get ready to run!
	self.__lockColumnsAndInit()

        self.subclassinit()
        
	if create:
	    self.db.createTable(self)

	if check:
	    self.db.checkTable(self)

    def getColumnDef(self,column_name):
        try:
            return self.__col_def_hash[column_name]
        except KeyError:
            try:
                return self.__vcol_def_hash[column_name]
            except KeyError:
                raise eNoSuchColumn, "no column (%s) on table %s" % (column_name,self.__table_name)

    def getColumnList(self):
        return self.__column_list + self.__vcolumn_list

    def databaseSizeForData_ColumnName_(self,data,col_name):
	try:
	    col_def = self.__col_def_hash[col_name]
	except KeyError:
	    try:
		col_def = self.__vcol_def_hash[col_name]
	    except KeyError:
		raise eNoSuchColumn, "no column (%s) on table %s" % (col_name,self.__table_name)

        c_name,c_type,c_options = col_def

        if c_type == kBigString:
            if c_options.get("compress_ok",0) and self.db.compression_enabled:
                z_size = len(zlib.compress(data,9))
                r_size = len(data)
                if z_size < r_size:
                    return z_size
                else:
                    return r_size
            else:
                return len(data)
        else:
            # really simplistic database size computation:
            try:
                a = data[0]
                return len(data)
            except:
                return 4
            

    def columnType(self, col_name):
	try:
	    col_def = self.__col_def_hash[col_name]
	except KeyError:
	    try:
		col_def = self.__vcol_def_hash[col_name]
	    except KeyError:
		raise eNoSuchColumn, "no column (%s) on table %s" % (col_name,self.__table_name)

	c_name,c_type,c_options = col_def
        return c_type

    def convertDataForColumn(self,data,col_name):
	try:
	    col_def = self.__col_def_hash[col_name]
	except KeyError:
	    try:
		col_def = self.__vcol_def_hash[col_name]
	    except KeyError:
		raise eNoSuchColumn, "no column (%s) on table %s" % (col_name,self.__table_name)

	c_name,c_type,c_options = col_def

        if c_type == kIncInteger:
            raise eInvalidData, "invalid operation for column (%s:%s) on table (%s)" % (col_name,c_type,self.__table_name)

	if c_type == kInteger:
	    try:
                if data is None: data = 0
                else: return long(data)
	    except (ValueError,TypeError):
		raise eInvalidData, "invalid data (%s) for col (%s:%s) on table (%s)" % (repr(data),col_name,c_type,self.__table_name)
	else:
	    if type(data) == type(long(0)):
		return "%d" % data
	    else:
		return str(data)

    def getPrimaryKeyList(self):
	return self.__primary_key_list
    
    def getTableName(self):
	return self.__table_name
    def hasValueColumn(self):
	return self.__has_value_column

    def hasColumn(self,name):
	return self.__col_def_hash.has_key(name)
    def hasVColumn(self,name):
	return self.__vcol_def_hash.has_key(name)
	

    def _defineRows(self):
	raise "can't instantiate base odb.Table type, make a subclass and override _defineRows()"

    def __lockColumnsAndInit(self):
	# add a 'odb_value column' before we lockdown the table def
	if self.__has_value_column:
	    self.d_addColumn("odb_value",kBigText,default='')

	self.__columns_locked = 1
	# walk column list and make lookup hashes, primary_key_list, etc..

	primary_key_list = []
	col_def_hash = {}
	for a_col in self.__column_list:
	    name,type,options = a_col
	    col_def_hash[name] = a_col
	    if options.has_key('primarykey'):
		primary_key_list.append(name)

	self.__col_def_hash = col_def_hash
	self.__primary_key_list = primary_key_list

	# setup the value columns!

	if (not self.__has_value_column) and (len(self.__vcolumn_list) > 0):
	    raise "can't define vcolumns on table without ValueColumn, call d_addValueColumn() in your _defineRows()"

	vcol_def_hash = {}
	for a_col in self.__vcolumn_list:
	    name,type,size_data,options = a_col
	    vcol_def_hash[name] = a_col

	self.__vcol_def_hash = vcol_def_hash
	
	
    def __checkColumnLock(self):
	if self.__columns_locked:
	    raise "can't change column definitions outside of subclass' _defineRows() method!"

    # table definition methods, these are only available while inside the
    # subclass's _defineRows method
    #
    # Ex:
    #
    # import odb
    # class MyTable(odb.Table):
    #   def _defineRows(self):
    #     self.d_addColumn("id",kInteger,primarykey = 1,autoincrement = 1)
    #     self.d_addColumn("name",kVarString,120)
    #     self.d_addColumn("type",kInteger,
    #                      enum_values = { 0 : "alive", 1 : "dead" }

    def d_addColumn(self,col_name,ctype,size=None,primarykey = 0, notnull = 0,indexed=0,
		    default=None,unique=0,autoincrement=0,safeupdate=0,enum_values = None,
                    relations=None,compress_ok=0,int_date=0):

	self.__checkColumnLock()

	options = {}
	options['default']       = default
	if primarykey:
	    options['primarykey']    = primarykey
	if indexed:
	    options['indexed']       = indexed
	if unique:
	    options['unique']        = unique
	if safeupdate:
	    options['safeupdate']    = safeupdate
	if autoincrement:
	    options['autoincrement'] = autoincrement
	if notnull:
	    options['notnull']       = notnull
	if size:
	    options['size']          = size
        if int_date:
            if ctype != kInteger:
                raise eInvalidData, "can't flag columns int_date unless they are kInteger"
            else:
                options['int_date'] = int_date
            
	if enum_values:
	    options['enum_values']   = enum_values
	    inv_enum_values = {}
	    for k,v in enum_values.items():
		if inv_enum_values.has_key(v):
		    raise eInvalidData, "enum_values paramater must be a 1 to 1 mapping for Table(%s)" % self.__table_name
		else:
		    inv_enum_values[v] = k
	    options['inv_enum_values'] = inv_enum_values
        if relations:
            options['relations']      = relations
            for a_relation in relations:
                table, foreign_column_name = a_relation
                if self.__relations_by_table.has_key(table):
                    raise eInvalidData, "multiple relations for the same foreign table are not yet supported" 
                self.__relations_by_table[table] = (col_name,foreign_column_name)
        if compress_ok:
            if ctype == kBigString:
                options['compress_ok'] = 1
            else:
                raise eInvalidData, "only kBigString fields can be compress_ok=1"
	
	self.__column_list.append( (col_name,ctype,options) )
	

    def d_addValueColumn(self):
	self.__checkColumnLock()
	self.__has_value_column = 1

    def d_addVColumn(self,col_name,type,size=None,default=None):
	self.__checkColumnLock()

	if (not self.__has_value_column):
	    raise "can't define VColumns on table without ValueColumn, call d_addValueColumn() first"

	options = {}
	if default:
	    options['default'] = default
	if size:
	    options['size']    = size

	self.__vcolumn_list.append( (col_name,type,options) )

    #####################
    # _checkColMatchSpec(col_match_spec,should_match_unique_row = 0)
    #
    # raise an error if the col_match_spec contains invalid columns, or
    # (in the case of should_match_unique_row) if it does not fully specify
    # a unique row.
    #
    # NOTE: we don't currently support where clauses with value column fields!
    #
    
    def _fixColMatchSpec(self,col_match_spec, should_match_unique_row = 0):
	if type(col_match_spec) == type([]):
	    if type(col_match_spec[0]) != type((0,)):
		raise eInvalidMatchSpec, "invalid types in match spec, use [(,)..] or (,)"
	elif type(col_match_spec) == type((0,)):
	    col_match_spec = [ col_match_spec ]
        elif type(col_match_spec) == type(None):
            if should_match_unique_row:
                raise eNonUniqueMatchSpec, "can't use a non-unique match spec (%s) here" % col_match_spec
            else:
                return None
	else:
	    raise eInvalidMatchSpec, "invalid types in match spec, use [(,)..] or (,)"

	if should_match_unique_row:
            unique_column_lists = []

            # first the primary key list
	    my_primary_key_list = []
	    for a_key in self.__primary_key_list:
		my_primary_key_list.append(a_key)

            # then other unique keys
            for a_col in self.__column_list:
                col_name,a_type,options = a_col
                if options.has_key('unique'):
                    unique_column_lists.append( (col_name, [col_name]) )

            unique_column_lists.append( ('primary_key', my_primary_key_list) )
                
	
	new_col_match_spec = []
	for a_col in col_match_spec:
	    name,val = a_col
	    # newname = string.lower(name)
	    #  what is this doing?? - jeske
	    newname = name
	    if not self.__col_def_hash.has_key(newname):
		raise eNoSuchColumn, "no such column in match spec: '%s'" % newname

	    new_col_match_spec.append( (newname,val) )

	    if should_match_unique_row:
                for name,a_list in unique_column_lists:
                    try:
                        a_list.remove(newname)
                    except ValueError:
                        # it's okay if they specify too many columns!
                        pass

	if should_match_unique_row:
            for name,a_list in unique_column_lists:
                if len(a_list) == 0:
                    # we matched at least one unique colum spec!
                    # log("using unique column (%s) for query %s" % (name,col_match_spec))
                    return new_col_match_spec
            
            raise eNonUniqueMatchSpec, "can't use a non-unique match spec (%s) here" % col_match_spec

	return new_col_match_spec

    def __buildWhereClause (self, col_match_spec,other_clauses = None):
	sql_where_list = []

        if not col_match_spec is None:
            for m_col in col_match_spec:
                m_col_name,m_col_val = m_col
                c_name,c_type,c_options = self.__col_def_hash[m_col_name]
                if c_type in (kIncInteger, kInteger):
                    try:
                        m_col_val_long = long(m_col_val)
                    except ValueError:
                        raise ValueError, "invalid literal for long(%s) in table %s" % (repr(m_col_val),self.__table_name)
                        
                    sql_where_list.append("%s = %d" % (c_name, m_col_val_long))
                else:
                    sql_where_list.append("%s = '%s'" % (c_name, self.db.escape(m_col_val)))

        if other_clauses is None:
            pass
        elif type(other_clauses) == type(""):
            sql_where_list = sql_where_list + [other_clauses]
        elif type(other_clauses) == type([]):
            sql_where_list = sql_where_list + other_clauses
        else:
            raise eInvalidData, "unknown type of extra where clause: %s" % repr(other_clauses)
                    
        return sql_where_list

    def __fetchRows(self,col_match_spec,cursor = None, where = None, order_by = None, limit_to = None,
                    skip_to = None, join = None):
	if cursor is None:
	    cursor = self.db.defaultCursor()

        # build column list
        sql_columns = []
        for name,t,options in self.__column_list:
            sql_columns.append(name)

        # build join information

        joined_cols = []
        joined_cols_hash = {}
        join_clauses = []
        if not join is None:
            for a_table,retrieve_foreign_cols in join:
                try:
                    my_col,foreign_col = self.__relations_by_table[a_table]
                    for a_col in retrieve_foreign_cols:
                        full_col_name = "%s.%s" % (my_col,a_col)
                        joined_cols_hash[full_col_name] = 1
                        joined_cols.append(full_col_name)
                        sql_columns.append( full_col_name )

                    join_clauses.append(" left join %s as %s on %s=%s " % (a_table,my_col,my_col,foreign_col))
                        
                except KeyError:
                    eInvalidJoinSpec, "can't find table %s in defined relations for %s" % (a_table,self.__table_name)
                    
        # start buildling SQL
    	sql = "select %s from %s" % (string.join(sql_columns,","),
                                     self.__table_name)

        # add join clause
        if join_clauses:
            sql = sql + string.join(join_clauses," ")
	
	# add where clause elements
        sql_where_list = self.__buildWhereClause (col_match_spec,where)
	if sql_where_list:
	    sql = sql + " where %s" % (string.join(sql_where_list," and "))

        # add order by clause
        if order_by:
            sql = sql + " order by %s " % string.join(order_by,",")

        # add limit
        if not limit_to is None:
            if not skip_to is None:
                sql = sql + " limit %s, %s" % (skip_to,limit_to)
            else:
                sql = sql + " limit %s" % limit_to
        else:
            if not skip_to is None:
                raise eInvalidData, "can't specify skip_to without limit_to in MySQL"

        dlog(DEV_SELECT,sql)
	cursor.execute(sql)

        # create defaultRowListClass instance...
        return_rows = self.__defaultRowListClass()
	    
	# should do fetchmany!
	all_rows = cursor.fetchall()
	for a_row in all_rows:
	    data_dict = {}

	    col_num = 0
            
            #	    for a_col in cursor.description:
            #		(name,type_code,display_size,internal_size,precision,scale,null_ok) = a_col
            for name in sql_columns:
		if self.__col_def_hash.has_key(name) or joined_cols_hash.has_key(name):
		    # only include declared columns!
                    if self.__col_def_hash.has_key(name):
                        c_name,c_type,c_options = self.__col_def_hash[name]
                        if c_type == kBigString and c_options.get("compress_ok",0) and a_row[col_num]:
                            try:
                                a_col_data = zlib.decompress(a_row[col_num])
                            except zlib.error:
                                a_col_data = a_row[col_num]

                            data_dict[name] = a_col_data
                        else:
                            data_dict[name] = a_row[col_num]

                    else:
                        data_dict[name] = a_row[col_num]
                        
		    col_num = col_num + 1

	    newrowobj = self.__defaultRowClass(self,data_dict,joined_cols = joined_cols)

	    return_rows.append(newrowobj)
	    
	return return_rows

    def __deleteRow(self,a_row,cursor = None):
	if cursor is None:
	    cursor = self.db.defaultCursor()

        # build the where clause!
        match_spec = a_row.getPKMatchSpec()
        sql_where_list = self.__buildWhereClause (match_spec)

        sql = "delete from %s where %s" % (self.__table_name,
                                           string.join(sql_where_list," and "))
        dlog(DEV_UPDATE,sql)
        cursor.execute(sql)
       

    def __updateRowList(self,a_row_list,cursor = None):
	if cursor is None:
	    cursor = self.db.defaultCursor()

	for a_row in a_row_list:
	    update_list = a_row.changedList()

	    # build the set list!
	    sql_set_list = []
	    for a_change in update_list:
		col_name,col_val,col_inc_val = a_change
		c_name,c_type,c_options = self.__col_def_hash[col_name]

                if c_type != kIncInteger and col_val is None:
                    sql_set_list.append("%s = NULL" % c_name)
                elif c_type == kIncInteger and col_inc_val is None:
                    sql_set_list.append("%s = 0" % c_name)
                else:
                    if c_type == kInteger:
                        sql_set_list.append("%s = %d" % (c_name, long(col_val)))
                    elif c_type == kIncInteger:
                        sql_set_list.append("%s = %s + %d" % (c_name,c_name,long(col_inc_val)))
                    elif c_type == kBigString and c_options.get("compress_ok",0) and self.db.compression_enabled:
                        compressed_data = zlib.compress(col_val,9)
                        if len(compressed_data) < len(col_val):
                            sql_set_list.append("%s = '%s'" % (c_name, self.db.escape(compressed_data)))
                        else:
                            sql_set_list.append("%s = '%s'" % (c_name, self.db.escape(col_val)))
                    else:
                        sql_set_list.append("%s = '%s'" % (c_name, self.db.escape(col_val)))

	    # build the where clause!
	    match_spec = a_row.getPKMatchSpec()
            sql_where_list = self.__buildWhereClause (match_spec)

	    if sql_set_list:
		sql = "update %s set %s where %s" % (self.__table_name,
						 string.join(sql_set_list,","),
						 string.join(sql_where_list," and "))

                dlog(DEV_UPDATE,sql)
                try:
                    cursor.execute(sql)
                except Exception, reason:
                    if string.find(str(reason), "Duplicate entry") != -1:
                        raise eDuplicateKey, reason
                    raise Exception, reason
		a_row.markClean()

    def __insertRow(self,a_row_obj,cursor = None):
	if cursor is None:
	    cursor = self.db.defaultCursor()

	sql_col_list = []
	sql_data_list = []
	auto_increment_column_name = None

	for a_col in self.__column_list:
	    name,type,options = a_col

	    try:
		data = a_row_obj[name]

		sql_col_list.append(name)
                if data is None:
                    sql_data_list.append("NULL")
                else:
                    if type == kInteger or type == kIncInteger:
                        sql_data_list.append("%d" % data)
                    elif type == kBigString and options.get("compress_ok",0) and self.db.compression_enabled:
                        compressed_data = zlib.compress(data,9)
                        if len(compressed_data) < len(data):
                            sql_data_list.append("'%s'" % self.db.escape(compressed_data))
                        else:
                            sql_data_list.append("'%s'" % self.db.escape(data))
                    else:
                        sql_data_list.append("'%s'" % self.db.escape(data))

	    except KeyError:
		if options.has_key("autoincrement"):
		    if auto_increment_column_name:
			raise eInternalError, "two autoincrement columns (%s,%s) in table (%s)" % (auto_increment_column_name, name,self.__table_name)
		    else:
			auto_increment_column_name = name
		
	
	sql = "insert into %s (%s) values (%s)" % (self.__table_name,
						   string.join(sql_col_list,","),
						   string.join(sql_data_list,","))

        dlog(DEV_UPDATE,sql)
        try:
          cursor.execute(sql)
        except Exception, reason:
          if string.find(str(reason), "Duplicate entry") != -1:
            raise eDuplicateKey, reason
          raise Exception, reason
            
	if auto_increment_column_name:
	    a_row_obj[auto_increment_column_name] = cursor.insert_id()

    # ----------------------------------------------------
    #   Helper methods for Rows...
    # ----------------------------------------------------


	
    #####################
    # r_deleteRow(a_row_obj,cursor = None)
    #
    # normally this is called from within the Row "delete()" method
    # but you can call it yourself if you want
    #

    def r_deleteRow(self,a_row_obj, cursor = None):
	curs = cursor
	self.__deleteRow(a_row_obj, cursor = curs)


    #####################
    # r_updateRow(a_row_obj,cursor = None)
    #
    # normally this is called from within the Row "save()" method
    # but you can call it yourself if you want
    #

    def r_updateRow(self,a_row_obj, cursor = None):
	curs = cursor
	self.__updateRowList([a_row_obj], cursor = curs)

    #####################
    # InsertRow(a_row_obj,cursor = None)
    #
    # normally this is called from within the Row "save()" method
    # but you can call it yourself if you want
    #

    def r_insertRow(self,a_row_obj, cursor = None):
	curs = cursor
	self.__insertRow(a_row_obj, cursor = curs)


    # ----------------------------------------------------
    #   Public Methods
    # ----------------------------------------------------


	
    #####################
    # deleteRow(col_match_spec)
    #
    # The col_match_spec paramaters must include all primary key columns.
    #
    # Ex:
    #    a_row = tbl.fetchRow( ("order_id", 1) )
    #    a_row = tbl.fetchRow( [ ("order_id", 1), ("enterTime", now) ] )


    def deleteRow(self,col_match_spec, where=None):
        n_match_spec = self._fixColMatchSpec(col_match_spec)
        cursor = self.db.defaultCursor()

        # build sql where clause elements
        sql_where_list = self.__buildWhereClause (n_match_spec,where)
        if not sql_where_list:
            return

        sql = "delete from %s where %s" % (self.__table_name, string.join(sql_where_list," and "))

        dlog(DEV_UPDATE,sql)
        cursor.execute(sql)
	
    #####################
    # fetchRow(col_match_spec)
    #
    # The col_match_spec paramaters must include all primary key columns.
    #
    # Ex:
    #    a_row = tbl.fetchRow( ("order_id", 1) )
    #    a_row = tbl.fetchRow( [ ("order_id", 1), ("enterTime", now) ] )


    def fetchRow(self, col_match_spec, cursor = None):
	n_match_spec = self._fixColMatchSpec(col_match_spec, should_match_unique_row = 1)

	rows = self.__fetchRows(n_match_spec, cursor = cursor)
	if len(rows) == 0:
	    raise eNoMatchingRows, "no row matches %s" % repr(n_match_spec)

	if len(rows) > 1:
	    raise eInternalError, "unique where clause shouldn't return > 1 row"

	return rows[0]
	    

    #####################
    # fetchRows(col_match_spec)
    #
    # Ex:
    #    a_row_list = tbl.fetchRows( ("order_id", 1) )
    #    a_row_list = tbl.fetchRows( [ ("order_id", 1), ("enterTime", now) ] )


    def fetchRows(self, col_match_spec = None, cursor = None, where = None, order_by = None, limit_to = None, skip_to = None, join = None):
	n_match_spec = self._fixColMatchSpec(col_match_spec)

	return self.__fetchRows(n_match_spec,
                                cursor = cursor,
                                where = where,
                                order_by = order_by,
                                limit_to = limit_to,
                                skip_to = skip_to,
                                join = join)

    def fetchRowCount (self, col_match_spec = None, cursor = None, where = None):
	n_match_spec = self._fixColMatchSpec(col_match_spec)

        sql_where_list = self.__buildWhereClause (n_match_spec,where)

    	sql = "select count(*) from %s" % self.__table_name
	if sql_where_list:
	    sql = "%s where %s" % (sql,string.join(sql_where_list," and "))

        if cursor is None:
          cursor = self.db.defaultCursor()
        dlog(DEV_SELECT,sql)
	cursor.execute(sql)
        try:
	    count, = cursor.fetchone()
        except TypeError:
            count = 0
        return count


    #####################
    # fetchAllRows()
    #
    # Ex:
    #    a_row_list = tbl.fetchRows( ("order_id", 1) )
    #    a_row_list = tbl.fetchRows( [ ("order_id", 1), ("enterTime", now) ] )

    def fetchAllRows(self):
        try:
            return self.__fetchRows([])
        except eNoMatchingRows:
            # else return empty list...
            return self.__defaultRowListClass()

    def newRow(self):
	row = self.__defaultRowClass(self,None,create=1)
        for (cname, ctype, opts) in self.__column_list:
            if opts['default'] is not None and ctype is not kIncInteger:
                row[cname] = opts['default']
        return row

class Row:
    __instance_data_locked  = 0
    def subclassinit(self):
        pass
    def __init__(self,_table,data_dict,create=0,joined_cols = None):

        self._inside_getattr = 0  # stop recursive __getattr__
	self._table = _table
	self._should_insert = create
        self._rowInactive = None
        self._joinedRows = []
	
	self.__pk_match_spec = None
	self.__vcoldata = {}
        self.__inc_coldata = {}

        self.__joined_cols_dict = {}
        for a_col in joined_cols or []:
            self.__joined_cols_dict[a_col] = 1
	
	if create:
	    self.__coldata = {}
	else:
	    if type(data_dict) != type({}):
		raise eInternalError, "rowdict instantiate with bad data_dict"
	    self.__coldata = data_dict
	    self.__unpackVColumn()

	self.markClean()

        self.subclassinit()
	self.__instance_data_locked = 1

    def joinRowData(self,another_row):
        self._joinedRows.append(another_row)

    def getPKMatchSpec(self):
	return self.__pk_match_spec

    def markClean(self):
	self.__vcolchanged = 0
	self.__colchanged_dict = {}

        for key in self.__inc_coldata.keys():
	    self.__coldata[key] = self.__coldata.get(key, 0) + self.__inc_coldata[key]

        self.__inc_coldata = {}

	if not self._should_insert:
	    # rebuild primary column match spec
	    new_match_spec = []
	    for col_name in self._table.getPrimaryKeyList():
		try:
		    rdata = self[col_name]
		except KeyError:
		    raise eInternalError, "must have primary key data filled in to save %s:Row(col:%s)" % (self._table.getTableName(),col_name)
		    
		new_match_spec.append( (col_name, rdata) )
	    self.__pk_match_spec = new_match_spec

    def __unpackVColumn(self):
	if self._table.hasValueColumn():
	    pass
	
    def __packVColumn(self):
	if self._table.hasValueColumn():
	    pass

    ## ----- utility stuff ----------------------------------

    def __del__(self):
	# check for unsaved changes
	changed_list = self.changedList()
	if len(changed_list):
            info = "unsaved Row for table (%s) lost, call discard() to avoid this error. Lost changes: %s\n" % (self._table.getTableName(), repr(changed_list)[:256])
            if 0:
                raise eUnsavedObjectLost, info
            else:
                sys.stderr.write(info)
                

    def __repr__(self):
	return "Row from (%s): %s" % (self._table.getTableName(),repr(self.__coldata) + repr(self.__vcoldata))

    ## ---- class emulation --------------------------------

    def __getattr__(self,key):
        if self._inside_getattr:
          raise AttributeError, "recursively called __getattr__ (%s,%s)" % (key,self._table.getTableName())
        try:
            self._inside_getattr = 1
            try:
                return self[key]
            except KeyError:
                if self._table.hasColumn(key) or self._table.hasVColumn(key):
                    return None
                else:
                    raise AttributeError, "unknown field '%s' in Row(%s)" % (key,self._table.getTableName())
        finally:
            self._inside_getattr = 0

    def __setattr__(self,key,val):
	if not self.__instance_data_locked:
	    self.__dict__[key] = val
	else:
	    my_dict = self.__dict__
	    if my_dict.has_key(key):
		my_dict[key] = val
	    else:
		# try and put it into the rowdata
		try:
		    self[key] = val
		except KeyError, reason:
		    raise AttributeError, reason


    ## ---- dict emulation ---------------------------------
    
    def __getitem__(self,key):
        self.checkRowActive()

        try:
            c_type = self._table.columnType(key)
        except eNoSuchColumn:
            # Ugh, this sucks, we can't determine the type for a joined
            # row, so we just default to kVarString and let the code below
            # determine if this is a joined column or not
            c_type = kVarString

        if c_type == kIncInteger:
            c_data = self.__coldata.get(key, 0) 
            if c_data is None: c_data = 0
            i_data = self.__inc_coldata.get(key, 0)
            if i_data is None: i_data = 0
            return c_data + i_data
        
	try:
	    return self.__coldata[key]
	except KeyError:
            try:
                return self.__vcoldata[key]
            except KeyError:
                for a_joined_row in self._joinedRows:
                    try:
                        return a_joined_row[key]
                    except KeyError:
                        pass

                raise KeyError, "unknown column %s in %s" % (key,self)

    def __setitem__(self,key,data):
        self.checkRowActive()
        
	try:
	    newdata = self._table.convertDataForColumn(data,key)
	except eNoSuchColumn, reason:
	    raise KeyError, reason

	if self._table.hasColumn(key):
	    self.__coldata[key] = newdata
	    self.__colchanged_dict[key] = 1
	elif self._table.hasVColumn(key):
	    self.__vcoldata[key] = newdata
	    self.__vcolchanged = 1
	else:
            for a_joined_row in self._joinedRows:
                try:
                    a_joined_row[key] = data
                    return
                except KeyError:
                    pass
	    raise KeyError, "unknown column name %s" % key

    def __delitem__(self,key,data):
        self.checkRowActive()
        
	if self.table.hasVColumn(key):
	    del self.__vcoldata[key]
	else:
            for a_joined_row in self._joinedRows:
                try:
                    del a_joined_row[key]
                    return
                except KeyError:
                    pass
	    raise KeyError, "unknown column name %s" % key


    def copyFrom(self,source):
        for name,t,options in self._table.getColumnList():
            if not options.has_key("autoincrement"):
                self[name] = source[name]


    # make sure that .keys(), and .items() come out in a nice order!

    def keys(self):
        self.checkRowActive()
        
        key_list = []
        for name,t,options in self._table.getColumnList():
            key_list.append(name)
        for name in self.__joined_cols_dict.keys():
            key_list.append(name)

        for a_joined_row in self._joinedRows:
            key_list = key_list + a_joined_row.keys()
            
        return key_list


    def items(self):
        self.checkRowActive()
        
        item_list = []
        for name,t,options in self._table.getColumnList():
            item_list.append( (name,self[name]) )
        for name in self.__joined_cols_dict.keys():
            item_list.append( (name,self[name]) )

        for a_joined_row in self._joinedRows:
            item_list = item_list + a_joined_row.items()


        return item_list

    def values(elf):
        self.checkRowActive()

        value_list = self.__coldata.values() + self.__vcoldata.values()

        for a_joined_row in self._joinedRows:
            value_list = value_list + a_joined_row.values()

        return value_list
        

    def __len__(self):
        self.checkRowActive()
        
	my_len = len(self.__coldata) + len(self.__vcoldata)

        for a_joined_row in self._joinedRows:
            my_len = my_len + len(a_joined_row)

        return my_len

    def has_key(self,key):
        self.checkRowActive()
        
	if self.__coldata.has_key(key) or self.__vcoldata.has_key(key):
	    return 1
	else:

            for a_joined_row in self._joinedRows:
                if a_joined_row.has_key(key):
                    return 1
	    return 0
	
    def get(self,key,default = None):
        self.checkRowActive()

        
        
	if self.__coldata.has_key(key):
	    return self.__coldata[key]
	elif self.__vcoldata.has_key(key):
	    return self.__vcoldata[key]
	else:
            for a_joined_row in self._joinedRows:
                try:
                    return a_joined_row.get(key,default)
                except eNoSuchColumn:
                    pass

            if self._table.hasColumn(key):
                return default
            
	    raise eNoSuchColumn, "no such column %s" % key

    def inc(self,key,count=1):
        self.checkRowActive()

        if self._table.hasColumn(key):
            try:
                self.__inc_coldata[key] = self.__inc_coldata[key] + count
            except KeyError:
                self.__inc_coldata[key] = count

            self.__colchanged_dict[key] = 1
        else:
            raise AttributeError, "unknown field '%s' in Row(%s)" % (key,self._table.getTableName())
    

    ## ----------------------------------
    ## real interface


    def fillDefaults(self):
	for field_def in self._table.fieldList():
	    name,type,size,options = field_def
	    if options.has_key("default"):
		self[name] = options["default"]

    ###############
    # changedList()
    #
    # returns a list of tuples for the columns which have changed
    #
    #   changedList() -> [ ('name', 'fred'), ('age', 20) ]

    def changedList(self):
	if self.__vcolchanged:
	    self.__packVColumn()

	changed_list = []
	for a_col in self.__colchanged_dict.keys():
	    changed_list.append( (a_col,self.get(a_col,None),self.__inc_coldata.get(a_col,None)) )

	return changed_list

    def discard(self):
	self.__coldata = None
	self.__vcoldata = None
	self.__colchanged_dict = {}
	self.__vcolchanged = 0

    def delete(self,cursor = None):
        self.checkRowActive()

        
        fromTable = self._table
        curs = cursor
        fromTable.r_deleteRow(self,cursor=curs)
        self._rowInactive = "deleted"

    def save(self,cursor = None):
	toTable = self._table

        self.checkRowActive()

	if self._should_insert:
	    toTable.r_insertRow(self)
	    self._should_insert = 0
	    self.markClean()  # rebuild the primary key list
	else:
            curs = cursor
	    toTable.r_updateRow(self,cursor = curs)

	# the table will mark us clean!
	# self.markClean()

    def checkRowActive(self):
        if self._rowInactive:
            raise eInvalidData, "row is inactive: %s" % self._rowInactive

    def databaseSizeForColumn(self,key):
        return self._table.databaseSizeForData_ColumnName_(self[key],key)

## -----------------------------------------------------------------------
##                            T  E  S  T S
## -----------------------------------------------------------------------

	
def TEST(output=log):
    LOGGING_STATUS[DEV_SELECT] = 1
    LOGGING_STATUS[DEV_UPDATE] = 1
    class AgentsTable(Table):
	def _defineRows(self):
	    self.d_addColumn("agent_id",kInteger,None,primarykey = 1,autoincrement = 1)
	    self.d_addColumn("login",kVarString,200,notnull=1)
	    self.d_addColumn("ext_email",kVarString,200,notnull=1)
	    self.d_addColumn("hashed_pw",kVarString,20,notnull=1)
	    self.d_addColumn("name",kBigString,compress_ok=1)
	    self.d_addColumn("auth_level",kInteger,None)
            self.d_addColumn("ticket_count",kIncInteger,None)


    import MySQLdb
    rdb = MySQLdb.connect(host = 'localhost',user='root', passwd = '', db='testdb')
    ndb = MySQLdb.connect(host = 'localhost',user='trakken', passwd = 'trakpas', db='testdb')
    db = Database(ndb)

    tbl = AgentsTable(db,"agents")

    cursor = rdb.cursor()

    # ---------------------------------------------------------------
    # initialize
    output("drop table agents")
    try:
        cursor.execute("drop table agents")   # clean out the table
    except MySQLdb.OperationalError:
        pass
    output("creating table")
    cursor.execute("create table agents (agent_id integer not null primary key auto_increment, login varchar(200) not null, unique (login), ext_email varchar(200) not null, hashed_pw varchar(20) not null, name varchar(200), auth_level integer default 0, ticket_count integer default 0)")

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
	raise "save and load failed"
    

    output("PASSED! change, save, load")

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

