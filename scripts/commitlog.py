#!/usr/bin/python

# commitlog.py
#
#
#  parse CVS commit logs and make nice submit logs...
#

import sys
import string


def main(argv):
  PATH = argv[1]

  body = sys.stdin.read()
  body_lines = string.split(body,"\n")

  mode = 0

  mod_files = []
  add_files = []
  log_lines = []

  for a_line in body_lines:
    if a_line == "Modified Files:":
      mode = "modfiles"
    elif a_line == "Added Files:":
      mode = "addfiles"
    elif a_line == "Log Message:":
      mode = "loglines"
    else:
      if mode == "modfiles":
        mod_files.append(string.strip(a_line))
      elif mode == "addfiles":
        add_files.append(string.strip(a_line))
      elif mode == "loglines":
        log_lines.append(a_line)

  log_summary = string.join(log_lines," ")[:60]

  fps = open(os.path.join(PATH,"neotonic.summary"),"a+")
  fps.write(log_summary + "\n")
  fps.close()

  log_data = ("-" * 80) + "\n" + body

  fp = open(os.path.join(PATH,"neotonic.summary"),"a+")
  fp.write(log_data)

if __name__ == "__main__::
  main(sys.argv)
