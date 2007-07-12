
import os, string, re, sys

# Check to see if the Egg system is installed (ie, setuptools)
# See http://peak.telecommunity.com/DevCenter/PythonEggs
USE_EGGS=1
try:
  from setuptools import setup
except ImportError:
  from distutils.core import setup
  USE_EGGS=0

from distutils.core import Extension
from distutils import sysconfig

VERSION = "0.10.5"
INC_DIRS = ["../"]
LIBRARIES = ["neo_cgi", "neo_cs", "neo_utl"]
LIB_DIRS = ["../libs"]
CC = "gcc"
LDSHARED = "gcc -shared"

## ARGGH!!  It looks like you can only specify a single item on the
## command-line or in the setup.cfg file for options which take multiple
## lists... and it overrides what is defined here.  So I have to do all
## the work of the configure file AGAIN here.  At least its in python,
## which is easier...
## Actually, forget that, I'm just going to load and parse the rules.mk
## file and build what I need

if not os.path.exists("../rules.mk"):
  raise "You need to run configure first to generate the rules.mk file!"

make_vars = { 'NEOTONIC_ROOT' : '..' }
rules = open("../rules.mk").read()
for line in string.split(rules, "\n"):
  parts = string.split(line, '=', 1)
  if len(parts) != 2: continue
  var, val = parts
  var = string.strip(var)
  make_vars[var] = val
  if var == "CFLAGS":
    matches = re.findall("-I(\S+)", val)
    inserted = []
    for inc_path in matches:
      # inc_path = match.group(1)
      if inc_path not in INC_DIRS:
      	inserted.append(inc_path)
	sys.stderr.write("adding inc_path %s\n" % inc_path)
    INC_DIRS = inserted + INC_DIRS
  elif var == "LIBS":
    matches = re.findall("-l(\S+)", val)
    inserted = []
    for lib in matches:
      # lib = match.group(1)
      if lib not in LIBRARIES:
      	inserted.append(lib)
	sys.stderr.write("adding lib %s\n" % lib)
    LIBRARIES = inserted + LIBRARIES
  elif var == "LDFLAGS":
    matches = re.findall("-L(\S+)", val)
    inserted = []
    for lib_path in matches:
      # lib_path = match.group(1)
      if lib_path not in LIB_DIRS:
      	inserted.append(lib_path)
	sys.stderr.write("adding lib_path %s\n" % lib_path)
    LIB_DIRS = inserted + LIB_DIRS
  elif var == "CC":
    CC = val
  elif var == "LDSHARED":
    LDSHARED = val


def expand_var(var, vars):
  def replace_var(m, variables=vars):
    var = m.group(1)
    if var[:2] == "$(" and var[-1] == ")":
      var = variables.get(var[2:-1], "")
    return var
  while 1:
    new_var = re.sub('(\$\([^\)]*\))', replace_var, var)
    if new_var == var: break
    var = new_var
  return var.strip()

def expand_vars(vlist, vars):
  nlist = []
  for val in vlist:
    val = expand_var(val, vars)
    if val: nlist.append(val)
  return nlist

INC_DIRS = expand_vars(INC_DIRS, make_vars)
LIB_DIRS = expand_vars(LIB_DIRS, make_vars)
LIBRARIES = expand_vars(LIBRARIES, make_vars)

CC = os.environ.get('CC', expand_var(CC, make_vars))
LDSHARED = os.environ.get('LDSHARED', expand_var(CC, make_vars))

# HACK!  The setup/Makefile may not have the hermetic/cross-compiler entries
# for the compiler that we need, so override them here!
given_cc = sysconfig.get_config_var('CC')
if given_cc != CC and given_cc[0] != '/':
  sys.stderr.write("Overriding setup's CC from %s to %s\n" % (given_cc, CC))
  try:
    sysconfig._config_vars['CC'] = CC
    sysconfig._config_vars['LDSHARED'] = LDSHARED
  except AttributeError:
    pass

setup_args = {
    'name': "clearsilver",
    'version': VERSION,
    'description': "Python ClearSilver Wrapper",
    'author': "Brandon Long",
    'author_email': "blong@fiction.net",
    'url': "http://www.clearsilver.net/",
    'ext_modules': [Extension(
      name="neo_cgi",
      sources=["neo_cgi.c", "neo_cs.c", "neo_util.c"],
      include_dirs=INC_DIRS,
      library_dirs=LIB_DIRS,
      libraries=LIBRARIES,
      )]
  }

if USE_EGGS:
  setup_args['zip_safe'] = 0

apply(setup, [], setup_args)
