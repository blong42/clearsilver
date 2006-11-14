

import MySQLdb
from hdfhelp import HdfRow, HdfItemList
from odb import *

class ABPeopleTable(Table):
  def _defineRows(self):
    self.d_addColumn("person_id", kInteger, primarykey = 1, autoincrement = 1)
    self.d_addColumn("fullname", kVarString, 255)
    self.d_addColumn("first_name", kVarString, 50)
    self.d_addColumn("last_name", kVarString, 50)
    self.d_addColumn("maiden_name", kVarString, 50)
    self.d_addColumn("title", kVarString, 50)
    self.d_addColumn("dob", kVarString, 14) # date
    self.d_addColumn("note", kVarString, 255)
    self.d_addColumn("primary_place_id", kInteger, 
                     relations = [('ab_places', 'place_id')])
    self.d_addColumn("primary_email_id", kInteger)
    self.d_addColumn("primary_phone_id", kInteger)

class ABPlaceTable(Table):
  def _defineRows(self):
    self.d_addColumn("place_id", kInteger, primarykey = 1, autoincrement = 1)
    self.d_addColumn("person_id", kInteger, default=0)
    self.d_addColumn("address", kVarString, 255)
    self.d_addColumn("city", kVarString, 255)
    self.d_addColumn("state", kFixedString, 2)
    self.d_addColumn("zip", kVarString, 15)
    self.d_addColumn("country", kVarString, 100)
    self.d_addColumn("valid_from", kVarString, 14) # date
    self.d_addColumn("valid_to", kVarString, 14) # date
    self.d_addColumn("note", kVarString, 255)

class ABEmailTable(Table):
  def _defineRows(self):
    self.d_addColumn("email", kVarString, 255, primarykey = 1)
    self.d_addColumn("person_id", kInteger, default=0)
    self.d_addColumn("last_received", kInteger) # timestamp
    self.d_addColumn("valid_from", kVarString, 14) # date
    self.d_addColumn("valid_to", kVarString, 14) # date
    self.d_addColumn("note", kVarString, 255)

class ABPhoneTable(Table):
  def _defineRows(self):
    self.d_addColumn("phone_id", kInteger, primarykey = 1, autoincrement = 1)
    self.d_addColumn("person_id", kInteger, default=0)
    self.d_addColumn("place_id", kInteger, default=0)
    self.d_addColumn("phone_number", kVarString, 20)
    self.d_addColumn("phone_type", kFixedString, 1, default='o',
                     enum_values = { 'm' : 'mobile',
                                     'h' : 'home',
                                     'b' : 'business',
                                     'p' : 'pager',
                                     'f' : 'fax',
                                     'o' : 'other'})
    self.d_addColumn("valid_from", kVarString, 14) # date
    self.d_addColumn("valid_to", kVarString, 14) # date
    self.d_addColumn("note", kVarString, 255)

class DB(Database):
  def __init__(self, db):
    Database.__init__(self, db)

    self.addTable("people", "ab_people", ABPeopleTable)
    self.addTable("places", "ab_places", ABPlaceTable)
    self.addTable("email", "ab_email", ABEmailTable)
    self.addTable("phone", "ab_phone", ABPhoneTable)

  def defaultRowClass(self):
    return HdfRow

  def defaultRowListClass(self):
    return HdfItemList

def connect(host = 'localhost'):
  db = MySQLdb.connect(host = host, user='blong', passwd='qwerty', db='blong')
  return DB(db)
