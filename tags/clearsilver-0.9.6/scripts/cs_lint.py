#!/usr/bin/python
#!/neo/opt/bin/python

import sys, string, os, getopt, signal, time
sys.path.append("../python")
import neo_cgi, neo_util
import cStringIO

class ClearSilverChecker:
  def __init__ (self):
    self.context = ""
    self.data = ""
    self.at = 0
    self.cmd = ""
    self.tokens = []

  def error(self, s):
    lineno = self.lineno(self.data, self.at)
    print "-E- [%s:%d] %s" % (self.context, lineno, s)
    if self.cmd:
      print "    Command is %s" % self.cmd
    if self.tokens:
      print "    Tokens: %s" % repr(self.tokens)

  def warn(self, s):
    lineno = self.lineno(self.data, self.at)
    print "-W- [%s:%d] %s" % (self.context, lineno, s)
    if self.cmd:
      print "    Command is %s" % self.cmd
    if self.tokens:
      print "    Tokens: %s" % repr(self.tokens)

  def check_file(self, filename):
    print "Checking file %s" % filename
    self.context = filename
    try:
      self.run_neo_cgi(filename)
    except neo_util.ParseError, reason:
      print "-E- %s" % str(reason)
    self.data = open(filename, "r").read()
    self.parse()

  def run_neo_cgi(self, filename):
    stdin = cStringIO.StringIO("")
    stdout = cStringIO.StringIO()
    neo_cgi.cgiWrap(stdin, stdout, {})
    neo_cgi.IgnoreEmptyFormVars(1)
    ncgi = neo_cgi.CGI()
    path = os.path.dirname(filename)
    ncgi.hdf.setValue("hdf.loadpaths.path", path)
    ncgi.display(filename)
    return 

  def lineno(self, data, i):
    return len(string.split(data[:i], '\n'))

  def parse(self):
    self.at = 0
    x = string.find(self.data[self.at:], '<?cs ')
    while x >= 0:
      self.at = x + self.at
      ce = string.find(self.data[self.at:], '?>')
      if ce == -1:
	self.error("Missing ?> in expression")
      else:
	ce = ce + self.at
	self.check_command(ce)
	
      # reset these class variables
      self.cmd = ""
      self.tokens = []
      self.at = self.at + 1
      x = string.find(self.data[self.at:], '<?cs ')

  def check_command(self, end):
    cmd = self.data[self.at+5:end]
    self.cmd = cmd
    if cmd[0] == '/':
      # handle end command
      cmd = cmd[1:]
      self.command_end(cmd)
      return

    pound = string.find(cmd, '#')
    colon = string.find(cmd, ':')
    bang = string.find(cmd, '!')
    if colon == -1 and bang == -1:
      if pound != -1:
	#print "Found comment: %s" % cmd
	pass
      else:
	self.command_begin(string.strip(cmd), "")
    elif pound != -1 and bang != -1 and pound < bang:
      # comment
      #print "Found comment: %s" % cmd
      pass
    elif pound != -1 and colon != -1 and pound < colon:
      # comment
      #print "Found comment: %s" % cmd
      pass
    elif bang == -1:
      arg = cmd[colon+1:]
      cmd = cmd[:colon]
      self.command_begin(cmd, arg)
    elif colon == -1:
      arg = cmd[bang+1:]
      cmd = cmd[:bang]
      self.command_begin(cmd, arg)

  def command_end(self, cmd):
    pass

  def command_begin(self, cmd, args):
    #print "%s -> %s" % (cmd, args)
    if cmd == "alt":
      self.check_expression(args)
    elif cmd == "if":
      self.check_expression(args)
    elif cmd == "elif":
      self.check_expression(args)
    elif cmd == "else":
      pass
    elif cmd == "include":
      self.check_expression(args)
    elif cmd == "linclude":
      self.check_expression(args)
    elif cmd == "name":
      self.check_expression(args)
    elif cmd == "var":
      self.check_expression(args)
    elif cmd == "evar":
      self.check_expression(args)
    elif cmd == "lvar":
      self.check_expression(args)
    elif cmd == "def":
      macro, args = self.split_macro(args)
      if macro: self.check_expression(macro, lvalue=1)
      if args:self.check_expression(args)
    elif cmd == "call":
      macro, args = self.split_macro(args)
      if macro: self.check_expression(macro, lvalue=1)
      if args:self.check_expression(args)
    elif cmd == "with":
      varname, args = self.split_equals(args)
      if varname: self.check_expression(varname, lvalue=1)
      if args: self.check_expression(args)
    elif cmd == "each":
      varname, args = self.split_equals(args)
      if varname: self.check_expression(varname, lvalue=1)
      if args: self.check_expression(args)
    elif cmd == "loop":
      varname, args = self.split_equals(args)
      if varname: self.check_expression(varname, lvalue=1)
      if args: self.check_expression(args)
    elif cmd == "set":
      varname, args = self.split_equals(args)
      if varname: self.check_expression(varname, lvalue=1)
      if args: self.check_expression(args)
    else:
      self.error("Unrecognized command %s" % cmd)

  def split_equals(self, args):
    x = string.find(args, '=')
    if x == -1:
      self.error("Missing equals")
      return None, None
    else:
      return args[:x], args[x+1:]

  def split_macro(self, args):
    b = string.find(args, '(')
    e = string.rfind(args, ')')
    if b == -1:
      self.error("Missing opening parenthesis")
      return None, None
    if e == -1:
      self.error("Missing closing parenthesis")
      return None, None
    macro_name = args[:b]
    args = args[b+1:e]
    return macro_name, args

  def check_expression(self, expr, lvalue=0):
    tokens = self.tokenize_expression(expr)
    #print repr(tokens)
    if len(tokens) == 0:
      self.error("Empty Expression")

  _OP = 1
  _VAR = 2
  _VARN = 3
  _STR = 4
  _NUM = 5

  _TOKEN_SEP = "\"?<>=!#-+|&,)*/%[]( \t\r\n"

  def tokenize_expression(self, expr):
    self.tokens = []
    while expr:
      #print "expr: '%s'" % expr
      expr = string.lstrip(expr)
      len_expr = len(expr)
      if len_expr == 0: break
      if expr[:2] in ["<=", ">=", "==", "!=", "||", "&&"]:
	self.tokens.append((ClearSilverChecker._OP, expr[:2]))
	expr = expr[2:]
	continue
      elif expr[0] in ["!", "?", "<", ">", "+", "-", "*", "/", "%", "(", ")", "[", "]", ".", ',']:
	self.tokens.append((ClearSilverChecker._OP, expr[0]))
	expr = expr[1:]
	continue
      elif expr[0] in ["#", "$"]:
	x = 1
	if expr[1] in ['+', '-']: x=2
	while len_expr > x and expr[x] not in ClearSilverChecker._TOKEN_SEP: x=x+1
	if x == 0:
	  self.error("[1] Zero length token, unexpected character %s" % expr[0])
	  x = 1
	else:
	  token = expr[1:x]
	  if expr[0] == "#":
	    try:
	      n = int(token)
	      t_type = ClearSilverChecker._NUM
	    except ValueError:
	      t_type = ClearSilverChecker._VARN
	  else:
	    t_type = ClearSilverChecker._VAR
	  self.tokens.append((t_type, token))
	expr = expr[x:]
	continue
      elif expr[0] in ['"', "'"]:
	x = string.find(expr[1:], expr[0])
	if x == -1:
	  self.error("Missing end of string %s " % expr)
	  break
	else:
	  x = x + 1
	  self.tokens.append((ClearSilverChecker._STR, expr[1:x]))
	  expr = expr[x+2:]
	  continue
      else:
	x = 0
	while len_expr > x and expr[x] not in ClearSilverChecker._TOKEN_SEP: x=x+1
	if x == 0:
	  self.error("[2] Zero length token, unexpected character %s" % expr[0])
	  x = 1
	else:
	  token = expr[:x]
	  try:
	    n = int(token)
	    t_type = ClearSilverChecker._NUM
	    self.warn("This behavior changed in version 0.9: previously this was a variable name, now its a number: %s" % token) 
	  except ValueError:
	    t_type = ClearSilverChecker._VAR
	  self.tokens.append((t_type, token))
	expr = expr[x:]
	continue
    return self.tokens

  # For version 0.9, we changed two things, we should check for them
  # both
  #  - an all numeric expression element is now considered a number and
  #    not an HDF variable name
  #  - we now use boolean evaluation in places that used to use either a
  #    special case or a numeric evaluation

def usage(argv0):
  print "%s: usage info!!" % argv0

def main(argv):
  alist, args = getopt.getopt(argv[1:], "", ["help"])

  for (field, val) in alist:
    if field == "--help":
      usage(argv[0])
      sys.exit(-1)

  for file in args:
    ClearSilverChecker().check_file(file)

if __name__ == "__main__":
  main(sys.argv)
