#!/usr/bin/python
# commitlog.py
#
#
#  parse CVS commit logs and make nice submit logs...
#

import sys
import os
import string
import time


def main(argv):
  PATH = argv[1]

  body = sys.stdin.read()
  body_lines = string.split(body,"\n")

  mode = 0

  mod_files = []
  add_files = []
  rem_files = []
  log_lines = []

  for a_line in body_lines:
    if a_line == "Modified Files:":
      mode = "modfiles"
    elif a_line == "Added Files:":
      mode = "addfiles"
    elif a_line == "Log Message:":
      mode = "loglines"
    elif a_line == "Removed Files:":
      mode = "remfiles"
    else:
      if mode == "modfiles":
        mod_files.append(string.strip(a_line))
      elif mode == "addfiles":
        add_files.append(string.strip(a_line))
      elif mode == "loglines":
        log_lines.append(a_line)
      elif mode == "remfiles":
        rem_files.append(string.strip(a_line))


  CVS_USER = os.environ['LOGNAME']
  DATE = time.strftime( "%I:%M%p %Y/%m/%d", time.localtime(time.time()))

  log_summary = "%10s %16s %s" % (CVS_USER,DATE,string.join(log_lines," ")[:60])

  filename = os.path.join(PATH,"neotonic.summary")
  fps = open(filename,"a+")
  fps.write(log_summary + "\n")
  fps.close()
  os.system('ci -q -l -m"none" %s %s,v' % (filename,filename))

  log_data = "----------------\n" + "USER: %s\n" % CVS_USER + "DATE: %s\n" % DATE + body

  filename = os.path.join(PATH,"neotonic")
  fp = open(filename,"a+")
  fp.write(log_data)
  fp.close()
  os.system('ci -q -l -m"none" %s %s,v' % (filename,filename))

if __name__ == "__main__":
  main(sys.argv)
