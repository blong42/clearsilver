

from odb import *
import profiler
import socket

USER = 'root'
PASSWORD = ''
DATABASE = 'trans_data'

class TransStringTable(Table):
    # access patterns:
    #   -> lookup individual entries by string_id
    #   -> lookup entry by string
    def _defineRows(self):
        self.d_addColumn("string_id", kInteger, primarykey=1, autoincrement=1)
        # we can't actually index this... but we can index part with myisam
        self.d_addColumn("string", kBigString, indexed=1)

## hmm, on second thought, storing this is in the database is kind 
## of silly..., since it essentially could change with each run.  It may
## not even be necessary to store this anywhere except in memory while 
## trans is running
class TransLocTable(Table):
    # access patterns:
    #   -> find "same" entry by filename/offset
    #   -> dump all locations for a version
    #   -> maybe: find all locations for a filename
    def _defineRows(self):
        self.d_addColumn("loc_id", kInteger, primarykey=1, autoincrement=1)
        self.d_addColumn("string_id", kInteger, indexed=1)
        self.d_addColumn("version", kInteger, default=0)
        self.d_addColumn("filename", kVarString, 255, indexed=1)
        self.d_addColumn("location", kVarString, 255)
        # this can either be:
        # ofs:x:y
        # hdf:foo.bar.baz

class TransMapTable(Table):
    # access patterns:
    #   -> dump all for a language
    #   -> lookup entry by string_id/lang
    def _defineRows(self):
        self.d_addColumn("string_id", kInteger, primarykey=1)
        self.d_addColumn("lang", kFixedString, 2, primarykey=1)
        self.d_addColumn("string", kBigString)

class DB(Database):
    def __init__(self, db, debug=0):
	self.db = db
        self._cursor = None
        self.debug = debug

        self.addTable("strings", "nt_trans_strings", TransStringTable)
        self.addTable("locs", "nt_trans_locs", TransLocTable)
        self.addTable("maps", "nt_trans_maps", TransMapTable)

    def defaultCursor(self):
        # share one cursor for this db object!
        if self._cursor is None:
            if self.debug:
                self._cursor = profiler.ProfilerCursor(self.db.cursor())
            else:
                self._cursor = self.db.cursor()

        return self._cursor

def trans_connect(host = 'localhost', debug=0):
    # try to optimize connection if on this machine
    if host != 'localhost':
        local_name = socket.gethostname()
        if string.find(local_name, '.') == -1:
            local_name = local_name + ".neotonic.com"
        if local_name == host:
            host = 'localhost'

    if debug: p = profiler.Profiler("SQL", "Connect -- %s:trans" % (host))
    db = MySQLdb.connect(host = host, user=USER, passwd = PASSWORD, db=DATABASE)
    if debug: p.end()

    retval = DB(db, debug=debug)
    return retval
