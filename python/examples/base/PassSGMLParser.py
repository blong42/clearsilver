##
## deja-vu batman, didn't I write this before?
## This parser is designed to parse an SGML document, and the default action 
## is just to pass the data through.  Based on TestSGMLParser from sgmllib.py
## Hmm, actually, make it a flag whether to handle unknown elements
##

from sgmllib import SGMLParser

class PassSGMLParser(SGMLParser):
  def __init__(self, fp, pass_unknown=0, verbose=0):
    self.pass_unknown = pass_unknown
    self.data = ""
    self.fp = fp
    SGMLParser.__init__(self, verbose)

  def handle_data(self, data):
    self.data = self.data + data

  def flush(self):
    data = self.data
    if data:
      self.data = ""
      self.write(data)

  def write (self, data):
    return self.fp.write(data)

  def write_starttag (self, tag, attrs):
    self.flush()
    if not attrs:
      self.write ("<%s>" % tag)
    else:
      self.write ("<" + tag)
      for name, value in attrs:
        self.write (" " + name + '=' + '"' + value + '"')
      self.write (">")

  def write_endtag (self, tag):
    self.flush()
    self.write ("</%s>" % tag)

  def handle_comment(self, data):
    # don't pass comments
    pass

  def unknown_starttag(self, tag, attrs):
    if self.pass_unknown:
      self.write_starttag (tag, attrs)

  def unknown_endtag(self, tag):
    if self.pass_unknown:
      self.write_endtag(tag)

  def handle_entityref(self, ref):
    self.flush()
    self.write ("&%s;" % ref)

  def handle_charref(self, ref):
    self.flush()
    self.write ("&#%s;" % ref)

  def close(self):
    SGMLParser.close(self)
    self.flush()
