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
    hdf_file = cgi.hdf.getValue("CGI.PathTranslated", "")
    if hdf_file == "":
      cgi.error ("No PATH_TRANSLATED var")
      return

    x = string.rfind (hdf_file, '/')
    if x != -1:
      cgi.hdf.setValue ("hdf.loadpaths.0", hdf_file[:x])

    cgi.hdf.readFile(hdf_file)
    content = cgi.hdf.getValue("Content", "")
    if content == "":
      cgi.error ("No Content var specified in HDF file %s" % hdf_file)
      return

    cgi.display(content)

  except neo_cgi.CGIFinished:
    return
  except Exception, Reason:
    log ("Python Exception: %s" % (str(repr(Reason))))
    s = neo_cgi.text2html("Python Exception: %s" % exceptionString())
    cgi.error (s)

if __name__ == "__main__":
  main (sys.argv, os.environ)
