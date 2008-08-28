#!/usr/bin/python2.4
# Copyright 2008, Google, Inc.
# Author: mjwiacek@google.com (Mike Wiacek)

import google3
import os

from google3.pyglib import flags
from google3.security.streamhtmlparser import py_html_parser
from google3.testing.pybase import googletest

PARSER_MODES = ('MODE_HTML', 'MODE_JS')

ATTRIBUTE_TYPES = ("ATTR_NONE", "ATTR_REGULAR", "ATTR_URI", "ATTR_JS", 
                  "ATTR_STYLE")

JAVASCRIPT_PARSER_STATES = ("JS_STATE_TEXT", "JS_STATE_Q", "JS_STATE_DQ",
                          "JS_STATE_REGEXP", "JS_STATE_COMMENT")

HTML_PARSER_STATES = ("HTML_STATE_TEXT", "HTML_STATE_TAG", "HTML_STATE_ATTR",
                    "HTML_STATE_VALUE", "HTML_STATE_COMMENT", 
                    "HTML_STATE_ERROR")

TEST_DATA_PATH = "google3/security/streamhtmlparser/testdata/"

class PyHtmlParserUnitTest(googletest.TestCase):

  def setUp(self):
    self.parser = py_html_parser.HtmlParser()
    self.test_data_path = TEST_DATA_PATH
    self.test_path = os.path.join(flags.FLAGS.test_srcdir, self.test_data_path)
    self.current_chunk = ""
    self.context_map = {}

  def LookupHtmlParserState(self, value):
    for i in range(len(HTML_PARSER_STATES)):
      if HTML_PARSER_STATES[i] == "HTML_STATE_" + value.upper():
        return i
    self.fail("Unable to lookup parser state by directive value")

  def LookupAttributeType(self, value):
    for i in range(len(ATTRIBUTE_TYPES)):
      if ATTRIBUTE_TYPES[i] == "ATTR_" + value.upper():
        return i
    self.fail("Unable to lookup attribute type")

  def LookupJavaScriptParserState(self, value):
    for i in range(len(JAVASCRIPT_PARSER_STATES)):
      if JAVASCRIPT_PARSER_STATES[i] == "JS_STATE_" + value.upper():
        return i
    self.fail("Unable to lookup javascript parser state by directive value")

  def ValidateState(self, value):
    state = self.LookupHtmlParserState(value)
    self.assertEqual(state, self.parser.State(), "Unexpected state!")

  def ValidateTag(self, value):
    tag = self.parser.Tag()
    self.assertNotEqual(tag, None, "Tag expected!")
    self.assertEqual(tag.lower(), value.lower())

  def ValidateAttribute(self, value):
    attribute = self.parser.Attribute()
    self.assertNotEqual(attribute, None, "Attribute expected!")
    self.assertEqual(attribute.lower(), value.lower())

  def ValidateValue(self, value):
    parser_value = self.parser.Value()
    self.assertNotEqual(parser_value, None, "Value expected!")
    self.assertEqual(value.lower(), parser_value.lower())

  def ValidateAttributeType(self, value):
    attr_type = self.LookupAttributeType(value)
    self.assertEqual(self.parser.AttributeType(), attr_type, 
                     "Unexpected attribute type!")

  def ValidateAttributeQuoted(self, value):
    if value.lower() == "true":
      self.assert_(self.parser.IsAttributeQuoted(),
                   "Attribute should be quoted")
    else:
      self.failIf(self.parser.IsAttributeQuoted(),
                  "Attribute should not be quoted")

  def ValidateInJavaScript(self, value):
    if value.lower() == "true":
      self.assert_(self.parser.InJavaScript(),
                   "Should be in JavaScript context")
    else:
      self.failIf(self.parser.InJavaScript(),
                  "Should not be in JavaScript context")

  def ValidateIsJavaScriptQuoted(self, value):
    if value.lower() == "true":
      self.assert_(self.parser.IsJavaScriptQuoted(),
                  "Unexpected unquoted JavaScript literal")
    else:
      self.failIf(self.parser.IsJavaScriptQuoted(),
                  "Expected a quoted JavaScript literal")

  def ValidateJavaScriptState(self, value):
    state = self.LookupJavaScriptParserState(value)
    self.assertEqual(state, self.parser.JavaScriptState(), 
                    "Unexpected JavaScript Parser state")

  def ValidateValueIndex(self, value):
    self.assertEqual(int(value), self.parser.ValueIndex(),
                     "Unexpected value index!")

  def processAnnotation(self, annotation):
    annotation = annotation.replace("<?state", "").replace("?>", "") 
    annotation = annotation.replace("\n", "").strip()
    pairs = [x.strip() for x in annotation.split(",")]
    for pair in pairs:
      if not pair:
        continue
      (first, second) = pair.lower().split("=")
      if first == "state":
        self.ValidateState(second)
      elif first == "tag":
        self.ValidateTag(second)
      elif first == "attr":
        self.ValidateAttribute(second)
      elif first == "value":
        self.ValidateValue(second)
      elif first == "attr_type":
        self.ValidateAttributeType(second)
      elif first == "attr_quoted":
        self.ValidateAttributeQuoted(second)
      elif first == "in_js":
        self.ValidateInJavaScript(second)
      elif first == "js_quoted":
        self.ValidateIsJavaScriptQuoted(second)
      elif first == "js_state":
        self.ValidateJavaScriptState(second)
      elif first == "value_index":
        self.ValidateValueIndex(second)
      elif first == "save_context":
        copy = py_html_parser.HtmlParser()
        copy.CopyFrom(self.parser)
        self.context_map[second] = copy
      elif first == "load_context":
        self.parser.CopyFrom(self.context_map[second])
      elif first == "reset":
        if second == "true":
          self.parser.Reset()
      elif first == "reset_mode":
        if second == "js":
          self.parser.ResetMode(py_html_parser.MODE_JS)
        elif second == "html":
          self.parser.ResetMode(py_html_parser.MODE_HTML)
        else:
          self.fail("Unknown mode type in reset_mode directive!")
      elif first == "insert_text":
        if second == "true":
          self.parser.InsertText()
      else:
        self.fail("Unknown test directive!")

  def ValidateFile(self, filename):
    directive_start = "<?state"
    directive_end = "?>"

    # Reset current parser
    self.parser.Reset()

    file_handle = open(self.test_path + filename, "r")
    data = file_handle.read()
    current_index = 0

    while current_index < len(data):
      start_index = data.find(directive_start, current_index)
      stop_index = data.find(directive_end, start_index) + len(directive_end)

      # Parse all text until the first directive
      if current_index > 0 and start_index < 0:
        self.parser.Parse(data[current_index:]) # No more annotations left
        self.current_chunk = data[current_index:]
        current_index += len(data[current_index:])
      else:
        self.parser.Parse(data[current_index:start_index])
        self.current_chunk = data[current_index:start_index]
        current_index += len(data[current_index:start_index])

      # Pull out the current annotation
      annotation = data[start_index:stop_index]
      self.current_chunk = annotation
      self.processAnnotation(annotation)
      current_index += len(annotation)

    return

  def testSimple(self):
    self.ValidateFile("simple.html")

  def testComments(self):
    self.ValidateFile("comments.html")

  def testJavaScriptBlock(self):
    self.ValidateFile("javascript_block.html")

  def testJavaScriptAttributes(self):
    self.ValidateFile("javascript_attribute.html")

  def testJavaScriptRegExp(self):
    self.ValidateFile("javascript_regexp.html")

  def testTags(self):
    self.ValidateFile("tags.html")

  def testContext(self):
    self.ValidateFile("context.html")

  def testReset(self):
    self.ValidateFile("reset.html")

  def testCData(self):
    self.ValidateFile("cdata.html")

  def testError(self):
    self.parser.Reset()
    self.assert_(self.parser.Parse("<a href='http://www.google.com' ''>\n"),
                 py_html_parser.HTML_STATE_ERROR)

if __name__ == '__main__':
  googletest.main()
