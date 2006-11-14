#!/neo/opt/bin/python

import sys, os, string, re, getopt, pwd, socket, time

def warn(*args):
  t = time.time()
  log_line = "[" + time.strftime("%m/%d %T", time.localtime(t)) + "] "
  l = []
  for arg in args:
    l.append(str(arg))
  log_line = log_line + string.join(l, " ") + "\n"
  sys.stderr.write(log_line)

class ChangeLog:
  def __init__ (self, module, release_from, release_to, copydir = None, cvsroot=None):
    self._module = module
    self._releaseFrom = release_from
    self._releaseTo = release_to
    self._cvsroot = cvsroot
    if cvsroot is None:
      self._cvsroot = os.environ.get("CVSROOT", None)

    self._copydir = copydir
    if copydir is None: 
      self._copydir = os.getcwd()
    self._names = {}

  def changeInfo (self):
    cmd = self.cvsCmd ("-q", "rdiff", "-s -r%s -r%s %s" % (self._releaseFrom, self._releaseTo, self._module))
    warn (cmd)
    fpi = os.popen (cmd)
    data = fpi.readlines()
    r = fpi.close()
    if r is None: r = 0
    if r != 0:
      warn ("Return code from command is %d\n" % r)
      return

    self.oldfiles = {}
    self.newfiles = []
    self.delfiles = []
    old_re = re.compile ("File (.*) changed from revision (.*) to (.*)")
    new_re = re.compile ("File (.*) is new; current revision (.*)")
    del_re = re.compile ("File (.*) is removed;")
    for line in data:
      m = old_re.match (line)
      if m:
        file = m.group(1)
        if file[:len(self._module)+1] == "%s/" % self._module:
          file = file[len(self._module)+1:]
        self.oldfiles[file] = (m.group(2), m.group(3))
        continue
      m = new_re.match (line)
      if m:
        file = m.group(1)
        if file[:len(self._module)+1] == "%s/" % self._module:
          file = file[len(self._module)+1:]
        self.newfiles.append(file)
        continue
      m = del_re.match (line)
      if m: 
        file = m.group(1)
        if file[:len(self._module)+1] == "%s/" % self._module:
          file = file[len(self._module)+1:]
        self.delfiles.append(file)
        continue
      warn ("Unknown response from changeInfo request:\n  %s" % line)

  def parselog (self, log):
    lines = string.split (log, '\n')
    in_header = 1
    x = 0
    num = len(lines)
    revisions = {}
    revision = None
    comment = []
    info_re = re.compile ("date: ([^; ]*) ([^;]*);  author: ([^;]*);")
    while (x < num):
      line = string.strip(lines[x])
      if line:
        if (x + 1 < num):
          nline = string.strip(lines[x+1])
        else:
          nline = None
        if in_header:
          (key, value) = string.split (line, ':', 1)
          if key == "Working file":
            filename = string.strip (value)
          elif key == "description":
            in_header = 0
        else:
          if (line == "----------------------------") and (nline[:9] == "revision "):
            if revision is not None:
              key = (date, author, string.join (comment, '\n'))
              try:
                revisions[key].append((filename, revision))
              except KeyError:
                revisions[key] = [(filename, revision)]
              comment = []
          elif line == "=" * 77:
            key = (date, author, string.join (comment, '\n'))
            try:
              revisions[key].append((filename, revision))
            except KeyError:
              revisions[key] = [(filename, revision)]
            in_header = 1
            revision = None
            comment = []
          elif line[:9] == "revision ":
            (rev, revision) = string.split (lines[x])
          else:
            m = info_re.match (lines[x])
            if m:
              date = m.group(1)
              author = m.group(3)
            else:
              comment.append (lines[x])
      x = x + 1
    return revisions

  def rcs2log (self):
    cwd = os.getcwd()
    os.chdir(self._copydir)
    files = string.join (self.oldfiles.keys(), ' ')
    cmd = 'rcs2log -v -r "-r%s:%s" %s' % (self._releaseFrom, self._releaseTo, files)
    fpi = os.popen (cmd)
    data = fpi.read()
    r = fpi.close()
    os.chdir(cwd)
    if r is None: r = 0
    if r != 0:
      warn (cmd)
      warn ("Return code from command is %d\n" % r)
      return

    fpo = open ("ChangeLog.%s" % self._releaseTo, 'w')
    fpo.write(data)
    fpo.close()

  def runCmd (self, cmd):
    cwd = os.getcwd()
    os.chdir(self._copydir)
    warn (cmd)
    fpi = os.popen (cmd)
    data = fpi.read()
    r = fpi.close()
    os.chdir(cwd)
    if r is None: r = 0
    if r != 0:
      warn ("Return code from command is %d\n" % r)
      return None
    return data

  def rcslog (self):
    inverted_log = {}
    if len(self.newfiles):
      cmd = self.cvsCmd ("", "log", "-N %s" % string.join(self.newfiles,' '))
      data = self.runCmd (cmd)
      if data is None: return
      revisions = self.parselog (data)
      for (key, value) in revisions.items():
        try:
          inverted_log[key] = inverted_log[key] + value
        except KeyError:
          inverted_log[key] = value

    filenames = string.join (self.oldfiles.keys(), ' ')
    if filenames:
      cmd = self.cvsCmd ("", "log", "-N -r%s:%s %s" % (self._releaseFrom, self._releaseTo, filenames))
      data = self.runCmd (cmd)
      if data is not None: 
        revisions = self.parselog (data)
        for (key, value) in revisions.items():
          for (filename, revision) in value:
            (rev1, rev2) = self.oldfiles[filename]
            if revision != rev1:
              try:
                inverted_log[key].append((filename, revision))
              except KeyError:
                inverted_log[key] = [(filename, revision)]

    fpo = open ("ChangeLog.%s" % self._releaseTo, 'w')
    fpo.write ("ChangeLog from %s to %s\n" % (self._releaseFrom, self._releaseTo))
    fpo.write ("=" * 72 + "\n")
    changes = inverted_log.items()
    changes.sort()
    changes.reverse()
    last_stamp = ""
    for (key, value) in changes:
      (date, author, comment) = key
      new_stamp = "%s  %s" % (date, self.fullname(author))
      if new_stamp != last_stamp:
        fpo.write ("%s\n\n" % new_stamp)
        last_stamp = new_stamp
      for (filename, revision) in value:
        fpo.write ("  * %s:%s\n" % (filename, revision))
      fpo.write ("    %s\n\n" % comment)
      
    fpo.close()

  def cvsCmd (self, cvsargs, cmd, cmdargs):
    root = ""
    if self._cvsroot is not None:
      root = "-d %s" % self._cvsroot

    cmd = "cvs -z3 %s %s %s %s" % (root, cvsargs, cmd, cmdargs)
    return cmd

  def fullname (self, author):
    try:
      return self._names[author]
    except KeyError:
      try:
        (name, passwd, uid, gid, gecos, dir, shell) = pwd.getpwnam(author)
        fullname = "%s  <%s@%s>" % (gecos, name, socket.gethostname())
      except KeyError:
        fullname = author

      self._names[author] = fullname
      return fullname
      

def usage (argv0):
  print "usage: %s [--help] module release1 release2" % argv0
  print __doc__

def main (argv, stdout, environ):
  list, args = getopt.getopt(argv[1:], "", ["help"])

  for (field, val) in list:
    if field == "--help":
      usage (argv[0])
      return

  if len (args) < 3:
    usage (argv[0])
    return

  cl = ChangeLog (args[0], args[1], args[2])
  cl.changeInfo()
  cl.rcslog()
  

if __name__ == "__main__":
  main (sys.argv, sys.stdout, os.environ)
