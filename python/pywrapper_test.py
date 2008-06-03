#!/usr/bin/python2.4
#

""" Test for python ClearSilver wrapper """

__author__ = 'blong@google.com (Brandon Long)'

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


def suite():
  return unittest.makeSuite(ClearsilverPythonWrapperTestCase, 'test')

if __name__ == '__main__':
  unittest.main()
