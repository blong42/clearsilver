
# who_calls.py
# by Sam Rushing for Medusa

import string
import sys

from log import *

whoCallsError = "whoCallsError"

#--------------------------------------------------------------
# Example use:
#
# import who_calls
# log(who_calls.pretty_who_calls())
#
#--------------------------------------------------------------

def test():
  for i in range(1,1000):
    pretty_who_calls()

  print_top_100()

def who_calls_helper():
  tinfo = []
  exc_info = sys.exc_info()

  f = exc_info[2].tb_frame.f_back
  while f:
	  tinfo.append ( (
		  f.f_code.co_filename,
		  f.f_code.co_name,
		  f.f_lineno )
		  )
	  f = f.f_back

  del exc_info
  return tinfo
  

def who_calls():
  try:
    raise whoCallsError
  except whoCallsError:
    tinfo = who_calls_helper()
  return tinfo

def pretty_who_calls(strip=0):
	info = who_calls()
	buf = []

	for file,function,line in info[1 + strip:]:
		buf.append("   %s(%s): %s()" % (file,line,function))
		
	return string.join(buf,"\n")

# ---------------------------------------------------------------------------
# used for debugging.
# ---------------------------------------------------------------------------

def compact_traceback ():
	t,v,tb = sys.exc_info()
	tbinfo = []
	if tb is None:
		# this should never happen, but then again, lots of things
		# should never happen but do.
		return (('','',''), str(t), str(v), 'traceback is None!!!')
	while 1:
		tbinfo.append (
			tb.tb_frame.f_code.co_filename,
			tb.tb_frame.f_code.co_name,				
			str(tb.tb_lineno)
			)
		tb = tb.tb_next
		if not tb:
			break

	# just to be safe
	del tb

	file, function, line = tbinfo[-1]
	info = '[' + string.join (
		map (
			lambda x: string.join (x, '|'),
			tbinfo
			),
		'] ['
		) + ']'

	return (file, function, line), str(t), str(v), info

## ----------------------------------------------------
## Refcount printing
		
import sys
import types

def real_get_refcounts(base = None, set_base = 0):
    d = {}
    sys.modules
    # collect all classes
    for modname,m in sys.modules.items():
        for sym in dir(m):
            o = getattr (m, sym)
            if type(o) is types.ClassType:
                name = "%s:%s" % (modname,o.__name__)
                cnt = sys.getrefcount (o)
                if base:
                    if set_base:
                        base[name] = cnt
                    elif cnt > base.get(name, 0):
                        d[name] = cnt - base.get(name, 0)
                else:
                    d[name] = cnt
    return d

def get_refcounts(base=None):
        d = real_get_refcounts(base = base)
        # sort by refcount
        pairs = map (lambda x: (x[1],x[0]), d.items())
        pairs.sort()
        pairs.reverse()
        return pairs

REFCOUNTS = {}

def set_refcount_base():
    global REFCOUNTS
    real_get_refcounts(REFCOUNTS, set_base = 1)

def print_top_100():
        print_top_N(100)

def print_top_N(N):
    global REFCOUNTS
    for n, c in get_refcounts(REFCOUNTS)[:N]:
       log('%10d %s' % (n, c))


