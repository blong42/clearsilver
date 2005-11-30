## another case of deja-vu
## this time, we want the slashdot style (what Yahoo said to do) only allow
## certain tags... we'll make it an option
## we'll have to tie this in some way to our HTML body displayer...
##
## Ok, there are basically four types of tags:
## 1) safe - ie, <b>, <i>, etc.
## 2) render problems - <table><form><body><frame> - these we either strip, 
##    or we have to ensure they match
## 3) definitely evil independent tags that we always strip
## 4) definitely evil tags which denote a region, we strip the entire region

from PassSGMLParser import PassSGMLParser
from urllib import basejoin
import string, sys
import neo_cgi

try:
  from cStringIO import StringIO
except:
  from StringIO import StringIO

class SafeHtml (PassSGMLParser):
  _safeTags = {"P":1, "LI":1, "DD":1, "DT":1, "EM":1, "BR":1, "CITE":1, 
               "DFN":1, "Q":1, "STRONG":1, "IMG":1, "HR":1,
               "TR":1, "TD":1, "TH":1, "CAPTION":1, "THEAD":1, "TFOOT":1, 
               "TBODY":1}
  _matchTags = {"TABLE":1, "OL":1, "UL":1, "DL":1, "CENTER":1, "DIV":1, "PRE":1,
                "SUB":1, "SUP":1, "BIG":1, "SMALL":1, "CODE":1,
                "B":1, "I":1, "A":1, "TT":1, "BLOCKQUOTE":1, "U":1,
                "H1":1, "H2":1, "H3":1, "H4":1, "H5":1, "H6":1, "FONT":1}
  _skipTags = {"FORM":1, "HTML":1, "BODY":1, "EMBED":1, "AREA":1, "MAP":1,
               "FRAME":1, "FRAMESET":1, "IFRAME":1, "META":1}
  _stripTags = {"HEAD":1, "JAVA":1, "APPLET":1, "OBJECT":1,
                "JAVASCRIPT":1, "LAYER":1, "STYLE":1, "SCRIPT":1}

  def __init__ (self, fp, extra_safe=1, base=None, map_urls=None, new_window=1):
    self._extra_safe = extra_safe
    PassSGMLParser.__init__ (self, fp, extra_safe)
    self._matchDict = {}
    self._stripping = 0
    self._base = base 
    self._map_urls = map_urls
    self._new_window = new_window

  def safe_start_strip (self):
    if self._stripping == 0:
      self.flush()
    self._stripping = self._stripping + 1

  def safe_end_strip (self):
    self.flush()
    self._stripping = self._stripping - 1
    if self._stripping < 0: self._stripping = 0

  def write (self, data):
    # sys.stderr.write("write[%d] %s\n" % (self._stripping, data))
    if self._stripping == 0:
      # sys.stderr.write("write %s\n" % data)
      PassSGMLParser.write(self, data)

  def cleanup_attrs (self, tag, attrs):
    new_attrs = [] 
    tag = string.lower(tag)
    if self._new_window and tag == "a":
        new_attrs.append(('target', '_blank'))
    for name, value in attrs:
      name = string.lower(name)
      if name[:2] == "on": continue   ## skip any javascript events
      if string.lower(value)[:11] == "javascript:": continue
      if self._map_urls and name in ["action", "href", "src", "lowsrc", "background"] and value[:4] == 'cid:':
        try:
          value = self._map_urls[value[4:]]
        except KeyError:
          pass
      else:
          if self._base and name in ["action", "href", "src", "lowsrc", "background"]:
            value = basejoin (self._base, value)
          if name in ["action", "href", "src", "lowsrc", "background"]:
            value = 'http://www.google.com/url?sa=D&q=%s' % (neo_cgi.urlEscape(value))
      if self._new_window and tag == "a" and name == "target": continue
      new_attrs.append ((name, value))
    return new_attrs

  def unknown_starttag(self, tag, attrs):
    tag = string.upper(tag)
    if SafeHtml._stripTags.has_key(tag):
      self.safe_start_strip()
      # sys.stderr.write("Stripping tag %s: %d\n" % (tag, self._stripping))
    elif SafeHtml._skipTags.has_key(tag):
      # sys.stderr.write("Skipping tag %s\n" % tag)
      pass
    elif SafeHtml._matchTags.has_key(tag):
      # sys.stderr.write("Matching tag %s\n" % tag)
      if self._matchDict.has_key(tag):
        self._matchDict[tag] = self._matchDict[tag] + 1
      else:
        self._matchDict[tag] = 1
      self.write_starttag (tag, self.cleanup_attrs(tag, attrs))
    elif SafeHtml._safeTags.has_key(tag):
      # sys.stderr.write("Safe tag %s\n" % tag)
      self.write_starttag (tag, self.cleanup_attrs(tag, attrs))
    elif not self._extra_safe:
      # sys.stderr.write("Other tag %s\n" % tag)
      self.write_starttag (tag, self.cleanup_attrs(tag, attrs))

  def unknown_endtag(self, tag):
    tag = string.upper(tag)
    if SafeHtml._stripTags.has_key(tag):
      self.safe_end_strip()
      # sys.stderr.write("End Stripping tag %s: %d\n" % (tag, self._stripping))
    elif self._stripping == 0:
      if SafeHtml._skipTags.has_key(tag):
        pass
      elif SafeHtml._matchTags.has_key(tag):
        if self._matchDict.has_key(tag):
          self._matchDict[tag] = self._matchDict[tag] - 1
        self.write_endtag (tag)
      elif SafeHtml._safeTags.has_key(tag):
        self.write_endtag (tag)
      elif not self._extra_safe:
        self.write_endtag (tag)

  def close (self):
    self._stripping = 0
    for tag in self._matchDict.keys():
      if self._matchDict[tag] > 0:
        for x in range (self._matchDict[tag]):
          self.write_endtag(tag)
    PassSGMLParser.close(self)

def SafeHtmlString (s, really_safe=1, map_urls=None):
#  fp = open("/tmp/safe_html.in", "w")
#  fp.write(s)
#  fp.close()
  fp = StringIO()
  parser = SafeHtml(fp, really_safe, map_urls=map_urls)
  parser.feed (s)
  parser.close ()
  s = fp.getvalue()
#  fp = open("/tmp/safe_html.out", "w")
#  fp.write(s)
#  fp.close()
  return s
  
