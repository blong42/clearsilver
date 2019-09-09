#!/usr/bin/python2.4
#

""" Test for python ClearSilver wrapper """

__author__ = 'blong@google.com (Brandon Long)'

try:
  from StringIO import StringIO
except ImportError:
  from io import StringIO
import unittest

import neo_cgi
import neo_cs
import neo_util

class ClearsilverPythonWrapperTestCase(unittest.TestCase):
  def testHtmlWhitespaceStripper(self):
    assert neo_cgi.htmlStripWhitespace("<html>     foo </html>", 1) == \
        '<html> foo </html>'
    assert neo_cgi.htmlStripWhitespace("<html>     foo </html>", 2) == \
        '<html> foo </html>'
    assert neo_cgi.htmlStripWhitespace("<html>     foo \n  \n\n</html>", 1) == \
        '<html> foo\n</html>'
    assert neo_cgi.htmlStripWhitespace("<html>     foo \n  \n\n</html>", 2) == \
        '<html> foo\n</html>'
    # Level 0 actually is handled in render/display, cgi_html_ws_strip doesn't
    # treat it differently
    assert neo_cgi.htmlStripWhitespace("<html>     foo \n  \n\n</html>", 0) == \
        '<html> foo\n</html>'

    assert neo_cgi.htmlStripWhitespace("<h>   foo \n   bar \n\n</h>", 1) == \
        '<h> foo\n  bar\n</h>'
    assert neo_cgi.htmlStripWhitespace("<h>   foo \n   bar \n\n</h>", 2) == \
        '<h> foo\nbar\n</h>'

  def testCsRenderStrip(self):
    hdf = neo_util.HDF()
    cs = neo_cs.CS(hdf)
    hdf.setValue("Foo.Bar", "1")
    cs.parseStr("This is my         file   <?cs var:Foo.Bar ?>   ")
    assert cs.render() == 'This is my         file   1   '
    hdf.setValue("ClearSilver.WhiteSpaceStrip", "1")
    assert cs.render() == 'This is my file 1 '

  def testJsEscape(self):
    assert neo_cgi.jsEscape("\x0A \xA9") == "\\x0A \xA9"

  def testJsonEscape(self):
    assert neo_cgi.jsonEscape("\x0A \xA9") == "\\u000A \xA9"
    assert neo_cgi.jsonEscape("PG&E") == "PG\\u0026E"

  def testValidateErrorString(self):
    fake_stdin = StringIO('')
    fake_stdout = StringIO()
    fake_env = {}
    neo_cgi.cgiWrap(fake_stdin, fake_stdout, fake_env)
    ncgi = neo_cgi.CGI()
    ncgi.error("%s")
    assert fake_stdout.getvalue().find("%s") != -1

  def testHdf(self):
    hdf = neo_util.HDF()
    hdf.setValue("Foo", "bar")
    assert hdf.getValue("Foo", "baz") == "bar"
    assert hdf.getValue("Foo.1", "baz") == "baz"
    hdf.setValue("Numbers.1", "1")
    hdf.setValue("Numbers.2", "2")
    hdf.setValue("Numbers.3", "3")
    assert hdf.getIntValue("Numbers.2", -1) == 2
    assert hdf.getIntValue("Numbers.5", -1) == -1
    hdf_num = hdf.getObj("Numbers")
    assert hdf_num.getIntValue("2", -1) == 2
    assert hdf_num.name() == "Numbers"
    assert hdf_num.child().name() == "1"
    assert hdf_num.child().value() == "1"
    hdf.setAttr("Numbers", "type", "integers")
    hdf.setAttr("Numbers", "k", "v")
    assert hdf.getAttrs("Numbers") == [('type', 'integers'), ('k', 'v')]
    assert hdf_num.attrs() == [('type', 'integers'), ('k', 'v')]

  def testMemorySafety(self):
    # This is meant to be run with ASAN and checks that
    # certain use-after-frees are not present.
    cgi = neo_cgi.CGI()
    hdf = cgi.hdf
    del cgi
    hdf.getValue("x", "y")
    del hdf

    cgi = neo_cgi.CGI()
    cs = cgi.cs()
    del cgi
    cs.parseStr("x")
    del cs

    hdf = neo_util.HDF()
    hdf.setValue("y.z", "1")
    child = hdf.getChild("y")
    del hdf
    child.getValue("x", "y")
    del child

    hdf = neo_util.HDF()
    cs = neo_cs.CS(hdf)
    del hdf
    cs.parseStr("x")
    del cs

def suite():
  return unittest.makeSuite(ClearsilverPythonWrapperTestCase, 'test')

if __name__ == '__main__':
  unittest.main()
