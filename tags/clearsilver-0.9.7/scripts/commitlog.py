#!/usr/bin/python
# commitlog.py
#
# Written July 2001 by David W. Jeske <jeske@neotonic.com>
#
# Released freely into the public domain.
#
#  parse CVS commit logs and make nice submit logs...
#

import sys
import os
import string
import time


def main(argv):
  PATH = argv[1]
  module = argv[2]

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


  # if files don't exist, we should create them...

  CVS_USER = os.environ['LOGNAME']
  DATE = time.strftime( "%I:%M%p %Y/%m/%d", time.localtime(time.time()))

  log_summary = "%10s %16s %s\n" % (CVS_USER,DATE,string.join(log_lines," ")[:60])

  filename = os.path.join(PATH,"%s.summary" % module)
  os.system('co -f -q -l %s %s,v' % (filename,filename))

  # check to see if the log line is already there
  fps = open(filename,"a+")
  try:
    fps.seek(-len(log_summary),2)
    check_data = fps.read(len(log_summary))
    if check_data != log_summary:
      fps.write(log_summary)
  except IOError:
    # Not enough data to go back that far
    fps.write(log_summary)
    

  fps.close()
  os.system('ci -q -m"none" %s %s,v' % (filename,filename))

  log_data = "----------------\n" + "USER: %s\n" % CVS_USER + "DATE: %s\n" % DATE + body

  filename = os.path.join(PATH,"%s" % module)
  os.system('co -f -q -l %s %s,v' % (filename,filename))
  fp = open(filename,"a+")
  fp.write(log_data)
  fp.close()
  os.system('ci -q -m"none" %s %s,v' % (filename,filename))

if __name__ == "__main__":
  main(sys.argv)
