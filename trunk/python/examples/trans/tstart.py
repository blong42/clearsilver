# this starts up the T-environment...
# 
# The root dir should point to the top of the python tree


import sys

ROOT_DIR = "../"
sys.path.insert(0, ROOT_DIR)

sys.path.insert(0, "../../") # pickup neo_cgi.so

try:
  from paths import paths
  sys.path = paths(ROOT_DIR) + sys.path
except:
  pass

# don't put anything above this because the path isn't
# extended yet...
