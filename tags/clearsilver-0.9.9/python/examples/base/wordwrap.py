"""
WordWrapping
"""


import os, sys, string, time, getopt
import re

def WordWrap(text, cols=70, detect_paragraphs = 0, is_header = 0):
  text =  string.replace(text,"\r\n", "\n") # remove CRLF
  def nlrepl(matchobj):
    if matchobj.group(1) != ' ' and matchobj.group(2) != ' ':
      repl_with = ' '
    else:
      repl_with = ''

    return matchobj.group(1) + repl_with + matchobj.group(2)

  if detect_paragraphs:
    text = re.sub("([^\n])\n([^\n])",nlrepl,text)

  body = []
  i = 0
  j = 0
  ltext = len(text)

  while i<ltext:
    if i+cols < ltext:
      r = string.find(text, "\n", i, i+cols)
      j = r
      if r == -1:
        j = string.rfind(text, " ", i, i+cols)
        if j == -1:
          r = string.find(text, "\n", i+cols)
          if r == -1: r = ltext
	  j = string.find(text, " ", i+cols)
	  if j == -1: j = ltext
          j = min(j, r)
    else:
      j = ltext

    body.append(string.strip(text[i:j]))
    i = j+1

  if is_header:
    body = string.join(body, "\n ")
  else:
    body = string.join(body, "\n")
  return body

