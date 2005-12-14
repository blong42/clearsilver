
import time
import who_calls
import neo_cgi

PROFILER_DATA = []
PROFILER_START = 0
PROFILER_ENABLED = 0
PROFILER_DEPTH = 0

def enable():
    global PROFILER_START
    global PROFILER_ENABLED
    global PROFILER_DATA
    PROFILER_START = time.time()
    PROFILER_ENABLED = 1
    PROFILER_DATA = []

def disable():
    global PROFILER_START
    global PROFILER_ENABLED
    global PROFILER_DATA
    PROFILER_START = 0
    PROFILER_ENABLED = 0
    PROFILER_DATA = []

def hdfExport(prefix, hdf):
    global PROFILER_DATA
    n = 0
    for p in PROFILER_DATA:
        hdf.setValue("%s.%d.when" % (prefix, n), "%5.2f" % (p.when))
        hdf.setValue("%s.%d.time" % (prefix, n), "%5.2f" % (p.length))
        hdf.setValue("%s.%d.klass" % (prefix, n), p.klass)
        hdf.setValue("%s.%d.what" % (prefix, n), "&nbsp;" * p.depth + p.what)
        hdf.setValue("%s.%d.where" % (prefix, n), neo_cgi.htmlEscape(p.where))

class Profiler:
    def __init__ (self, klass, what):
        global PROFILER_START
        global PROFILER_ENABLED
        global PROFILER_DATA
        global PROFILER_DEPTH
        if not PROFILER_ENABLED: return
        self.when = time.time() - PROFILER_START
        self.klass = klass
        self.where = who_calls.pretty_who_calls()
        self.what = what
        self.length = 0
        self.depth = PROFILER_DEPTH
        PROFILER_DEPTH = PROFILER_DEPTH + 1

        PROFILER_DATA.append(self)

    def end(self):
        global PROFILER_ENABLED
        global PROFILER_DEPTH
        if not PROFILER_ENABLED: return
        self.length = time.time() - self.when - PROFILER_START
        PROFILER_DEPTH = PROFILER_DEPTH - 1
        if PROFILER_DEPTH < 0: PROFILER_DEPTH = 0

class ProfilerCursor:
    def __init__ (self, real_cursor):
        self.real_cursor = real_cursor

    def execute (self, query, args=None):
        p = Profiler("SQL", query)
        r = self.real_cursor.execute(query, args)
        p.end()
        return r

    def __getattr__ (self, key):
        return getattr(self.real_cursor, key)
