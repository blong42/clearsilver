#!/usr/bin/env python

import sys, os, traceback, string
import neo_cgi

def log (s):
  sys.stderr.write("CGI: %s\n" % s)

def exceptionString():
  import StringIO

  ## get the traceback message  
  sfp = StringIO.StringIO()
  traceback.print_exc(file=sfp)
  exception = sfp.getvalue()
  sfp.close()

  return exception

def main (argv, environ):
  # log ("starting")
  cgi = neo_cgi.CGI("")

  try:
    fp = cgi.filehandle("file")
    print "Content-Type: text/plain\r\n\r\n"
    data = fp.read()
    print data

    f = open("/tmp/file", "w")
    f.write(data)

  except neo_cgi.CGIFinished:
    return
  except Exception, Reason:
    log ("Python Exception: %s" % (str(repr(Reason))))
    s = neo_cgi.text2html("Python Exception: %s" % exceptionString())
    cgi.error (s)

if __name__ == "__main__":
  main (sys.argv, os.environ)
