#!/neo/opt/bin/python

# log.py

import sys, time

DEV = "development"
DEV_UPDATE = "update queries"
DEV_SELECT = "select queries"
DEV_REPORT = "report log"

LOGGING_STATUS = {
   DEV : 1,
   DEV_UPDATE : 0,
   DEV_SELECT : 0,
   DEV_REPORT : 0}

tstart = 0

def dlog(when,astr):
    global LOGGING_STATUS
    try:
        if LOGGING_STATUS[when]:
            log(astr)
    except KeyError:
        pass

def tlog(astr):
    global tstart
    t = time.time()
    if tstart == 0:
        tstart = t
    time_stamp = "%5.5f" % (t-tstart)
    if len(astr):
        if astr[-1] == "\n":
            sys.stderr.write("[%s] %s" % (time_stamp, astr))
        else:
            sys.stderr.write("[%s] %s\n" % (time_stamp, astr))

def log(astr):
    if len(astr) > 1024:
        astr = astr[:1024]

    t = time.time()
    time_stamp = time.strftime("%m/%d %T", time.localtime(t))
    if len(astr):
        if astr[-1] == "\n":
            sys.stderr.write("[%s] %s" % (time_stamp, astr))
        else:
            sys.stderr.write("[%s] %s\n" % (time_stamp, astr))
    # sys.stderr.flush()
