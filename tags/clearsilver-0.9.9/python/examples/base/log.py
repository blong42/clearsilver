#!/neo/opt/bin/python

# log.py

import sys, time, string

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

def orig_log(astr):
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


#----------------------------------------------------------------------
# static functions

_gDebug = 0
_gFileDebug = 0

kBLACK = 0
kRED = 1
kGREEN = 2
kYELLOW = 3
kBLUE = 4
kMAGENTA = 5
kCYAN = 6
kWHITE = 7
kBRIGHT = 8

def ansicolor (str, fgcolor = None, bgcolor = None):
  o = ""
  if fgcolor:
    if fgcolor & kBRIGHT:
      bright = ';1'
    else:
      bright = ''
    o = o + '%c[3%d%sm' % (chr(27), fgcolor & 0x7, bright)
  if bgcolor:
    o = o + '%c[4%dm' % (chr(27), bgcolor)
  o = o + str
  if fgcolor or bgcolor:
    o = o + '%c[0m' % (chr(27))
  return o


def _log(*args):
  t = time.time()

  log_line = ""

  log_line = log_line + "[" + time.strftime("%m/%d %T", time.localtime(t)) + "] "

  l = []
  for arg in args:
    l.append(str(arg))
  log_line = log_line + string.join(l, " ") + "\n"

  sys.stderr.write(log_line)

def warn(*args):
  apply(_log, args)

def warnred(*args):
  args = tuple (["[31m"] + list(args) + ["[0m"])
  apply(_log, args)

def log(*args):
  apply(_log, args)

def logred(*args):
  if _gDebug>=1: 
    args = tuple (["[31m"] + list(args) + ["[0m"])
    apply(_log, args)

def debug(*args):
  if _gDebug>=2: apply(_log, args)


def debugfull():
  global _gDebug
  _gDebug = 2

def debugon():
  global _gDebug
  _gDebug = 1

def debugoff():
  global _gDebug
  _gDebug = 0

