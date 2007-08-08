#!/usr/bin/env python
"""
  document.py -- Simple script to generate manpages from C header
  files.  Looks for the following formatted C comments in the C header files:

  /*
   * Function: my_function - This is my function
   * Description: My function does the following things, in no particular 
   *              order: It eats, sleeps, and is merry
   * Input: arg1 - This argument is healthy
   *        arg2 - This argument is wealthy
   *        arg3 - This argument is wise
   * Output: arg4 - The location of the porridge
   *         arg5 - The location of the spider
   * Returns: -1 on error, 0 otherwise
   */

"""


import sys, os, getopt, string, re, time

QUIET = 0

def usage(argv0):
  print "%s [--help]" % argv0

class FuncDoc:
  def __init__ (self, name):
    self._name = name
    self._title = None
    self._desc = None
    self._args = None
    self._retr = None
    self._defn = None
    self._output = None
    self._other = ""

  def __repr__(self):
    out = []
    out.append("Name: %s" % self._name)
    if self._title is not None:
      out.append("Title: %s" % self._title)
    if self._desc is not None:
      out.append("Description: %s" % self._desc)
    if self._args is not None:
      out.append("Input: %s" % self._args)
    if self._output is not None:
      out.append("Output: %s" % self._output)
    if self._retr is not None:
      out.append("Returns: %s" % self._retr)
    if string.strip(self._other):
      out.append("Other: %s" % self._other)
    if self._defn is not None:
      out.append("Definition:")
      out.append(self._defn)
    return string.join(out, "\n")

class CParser:
  STATE_OTHER = 0
  STATE_COMT = 1
  STATE_FUNC = 2

  RE_C_comment = re.compile("/\*(.*)")
  RE_C_define = re.compile("\s*#\s*define (\S+) (.*)")
  RE_C_typedef = re.compile("typedef (\S+) (.*)")
  RE_C_func_def = re.compile("[^#]*(\S+)([ \*]+)(\S+)\s*\([^\)]*\);")
  RE_C_func_def_b = re.compile("[^#]*(\S+)([ \*]+)(\S+)\s*\([^\)]*")
  RE_C_func_com = re.compile("function:\s*(\S+)(.*)", re.IGNORECASE) 
  RE_C_desc_com = re.compile("description:\s*(.+)", re.IGNORECASE) 
  RE_C_args_com = re.compile("(arguments|input):\s*(.+)", re.IGNORECASE) 
  RE_C_retr_com = re.compile("(return|returns):\s*(.+)", re.IGNORECASE) 
  RE_C_out_com = re.compile("output:\s*(.+)", re.IGNORECASE)
  RE_C_other_com = re.compile("(\S+):\s*(.+)")
  RE_C_com_cont = re.compile("[ \*]*(.+)")

  def __init__ (self, filename):
    self._filename = filename
    self._funcs = {}

  def func (self, name):
    try:
      return self._funcs[name]
    except KeyError:
      f = FuncDoc(name)
      self._funcs[name] = f
      return f

  def go(self):
    try:
        fp = open(self._filename)
    except IOError:
        return
    state = CParser.STATE_OTHER
    f = None
    cont = None
    while 1:
      line = fp.readline()
      if not line: break
      if state == CParser.STATE_OTHER:
        m = CParser.RE_C_comment.search (line)
        if m:
          line = m.group(1)
          state = CParser.STATE_COMT
        else:
          m = CParser.RE_C_define.match(line)
          if m: continue
          m = CParser.RE_C_typedef.search(line)
          if m: continue
          m = CParser.RE_C_func_def.match(line)
          if m:
            func_name = m.group(3)
            f = self.func(func_name)
            f._defn = line
          else:
            m = CParser.RE_C_func_def_b.match(line)
            if m:
              state = CParser.STATE_FUNC
              func_name = m.group(3)
              f = self.func(func_name)
              f._defn = line
              continue
      if state == CParser.STATE_COMT:
        if string.find(line, "*/") != -1:
          state = CParser.STATE_OTHER
          continue
        m = CParser.RE_C_func_com.search(line)
        if m:
          cont = "func"
          f = self.func(m.group(1))
          f._title = m.group(2)
          continue
        m = CParser.RE_C_desc_com.search(line)
        if m:
          cont = "desc"
          f._desc = m.group(1)
          continue
        m = CParser.RE_C_args_com.search(line)
        if m:
          cont = "args"
          f._args = m.group(2)
          continue
        m = CParser.RE_C_retr_com.search(line)
        if m:
          cont = "retr"
          f._retr = m.group(2)
          continue
        m = CParser.RE_C_out_com.search(line)
        if m:
          cont = "out"
          f._output = m.group(1)
          continue
        m = CParser.RE_C_other_com.search(line)
        if not f: continue
        if m:
          cont = "other"
          f._other = f._other + "%s: %s" % (m.group(1), m.group(2))
          continue
        m = CParser.RE_C_com_cont.search(line)
        if m:
          if cont == "func":
            f._title = f._title + '\n' + m.group(1)
          elif cont == "desc":
            f._desc = f._desc + '\n'+ m.group(1)
          elif cont == "args":
            f._args = f._args + '\n' + m.group(1)
          elif cont == "retr":
            f._retr = f._retr + '\n' + m.group(1)
          elif cont == "out":
            f._output = f._output + '\n' + m.group(1)
          elif cont == "other":
            f._other = f._other + '\n' + m.group(1)
      elif state == CParser.STATE_FUNC:
        f._defn = f._defn+line
        if string.find(line, ");") != -1:
          state = CParser.STATE_OTHER

  def dump(self):
    for name in self._funcs.keys():
      # print name
      print "%s\n" % self._funcs[name]

  def dump_manpages(self, directory, owner):
    global QUIET
    date = time.strftime("%d %B %Y", time.localtime(time.time()))
    for name, f in self._funcs.items():
      if f._title is None and f._desc is None and f._args is None and f._retr is None:
        if not QUIET:
          sys.stderr.write('-W- No info for function "%s()"\n' % name)
        continue
      if f._defn is None:
        if not QUIET:
          sys.stderr.write('-W- No defn for function "%s()"\n' % name)
      fp = open("%s/%s.3" % (directory, name), "w")
      fp.write('.TH %s 3 "%s" "%s" "%s"\n\n' % (name, date, owner, self._filename))
      fp.write('.de Ss\n.sp\n.ft CW\n.nf\n..\n')
      fp.write('.de Se\n.fi\n.ft P\n.sp\n..\n')
      fp.write('.SH NAME\n')
      if f._title is None:
        fp.write('%s\n' % f._name)
      else:
        fp.write('%s %s\n' % (f._name, f._title))
      fp.write('.SH SYNOPSIS\n')
      fp.write('.Ss\n#include <%s>\n.Se\n' % self._filename)
      if f._defn:
        fp.write('.Ss\n%s\n.Se\n' % f._defn)
      else:
        fp.write('.Ss\n%s()\n.Se\n' % f._name)
      fp.write('\n')
      if f._args:
        fp.write('.SH ARGUMENTS\n')
        fp.write('%s\n\n' % string.replace(f._args, '\n', '\n.br\n'))
      if f._desc or string.strip(f._other):
        fp.write('.SH DESCRIPTION\n')
        if f._desc: fp.write('%s\n\n' % f._desc)
        if string.strip(f._other): fp.write('%s\n\n' % f._other)
      if f._output:
        fp.write('.SH "RETURN VALUE"\n')
        fp.write('%s\n\n' % string.replace(f._output, '\n', '\n.br\n'))
      fp.write('.SH "SEE ALSO"\n')
      fp.write('.BR %s\n' % string.join(self._funcs.keys(), ' "(3), "'))
      fp.close()

  def dump_hdf (self, directory, owner):
    global QUIET
    sys.path.insert (0, "../python")
    sys.path.insert (0, "python")
    import neo_cgi, neo_util
    hdf = neo_util.HDF()
    date = time.strftime("%d %B %Y", time.localtime(time.time()))
    if not self._funcs.items(): return
    for name, f in self._funcs.items():
      if f._title is None and f._desc is None and f._args is None and f._retr is None:
        if not QUIET:
          sys.stderr.write('-W- No info for function "%s()"\n' % name)
        continue
      if f._defn is None:
        if not QUIET:
          sys.stderr.write('-W- No defn for function "%s()"\n' % name)
      hdf.setValue ("Code.%s" % name, name)
      obj = hdf.getObj ("Code.%s" % name)
      obj.setValue ("Name", name)
      obj.setValue ("filename", self._filename)
      if f._title: obj.setValue ("Title", f._title)
      if f._defn: obj.setValue ("Define", neo_cgi.text2html(f._defn))
      if f._args: obj.setValue ("Args", neo_cgi.text2html(f._args))
      if f._desc: obj.setValue ("Desc", neo_cgi.text2html(f._desc))
      if string.strip(f._other): obj.setValue ("Other", neo_cgi.text2html(string.strip(f._other)))
      if f._output: obj.setValue ("Output", neo_cgi.text2html(f._output))
      n = 0
      for func in self._funcs.keys():
        obj.setValue ("related.%d" % n, func)
        n = n + 1

    fname = self._filename
    x = string.rindex (fname, "/")
    if x != -1: fname = fname[x+1:]
    x = string.rindex (fname, '.')
    if x != -1: fname = fname[:x]

    hdf.writeFile ("%s/%s.hdf" % (directory, fname))

def main(argv, environ):
  alist, args = getopt.getopt(argv[1:], "q", ["help", "outdir=", "owner=", "hdf"])

  outdir = "."
  owner = ""
  do_hdf = 0
  for (field, val) in alist:
    if field == "--help":
      usage (argv[0])
      return
    if field == "--outdir":
      outdir = val
    if field == "--owner":
      owner = val
    if field == "-q":
      global QUIET
      QUIET = 1
    if field == "--hdf":
      do_hdf = 1

  if args:
    for file in args:
      parser = CParser(file)
      parser.go()
      if not do_hdf:
        parser.dump_manpages(outdir, owner)
      else:
        parser.dump_hdf (outdir, owner)


if __name__ == "__main__":
  main (sys.argv, os.environ)
