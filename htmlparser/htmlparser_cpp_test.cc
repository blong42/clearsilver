// Copyright 2007 Google Inc. All Rights Reserved.
// Author: falmeida@google.com (Filipe Almeida)
//
// Verify at different points during HTML processing that the parser is in the
// correct state.
//
// The annotated file consists of regular html blocks and html processing
// instructions with a target name of "state" and a list of comma separated key
// value pairs describing the expected state or invoking a parser method.
// Example:
//
// <html><body><?state state=text, tag=body ?>
//
// For a more detailed explanation of the acceptable values please consult
// htmlparser_cpp.h. Following is a list of the possible keys:
//
// state: Current parser state as returned by HtmlParser::state().
//        Possible values: text, tag, attr, value, comment or error.
// tag: Current tag name as returned by HtmlParser::tag()
// attr: Current attribute name as returned by HtmlParser::attr()
// attr_type: Current attribute type as returned by HtmlParser::attr_type()
//            Possible values: none, regular, uri, js or style.
// attr_quoted: True if the attribute is quoted, false if it's not.
// in_js: True if currently processing javascript (either an attribute value
//        that expects javascript, a script block or the parser being in
//        MODE_JS)
// js_quoted: True if inside a javascript string literal.
// js_state: Current javascript state as returned by
//           HtmlParser::javascript_state().
//           Possible values: text, q, dq, regexp or comment.
// value_index: Integer value containing the current character index in the
//              current value starting from 0.
// reset: If true, resets the parser state to it's initial values.
// reset_mode: Similar to reset but receives an argument that changes the
//             parser mode into either mode html or mode js.
// insert_text: Executes HtmlParser::InsertText() if the argument is true.

#include <string>
#include <utility>
#include <vector>
#include <hash_map>
#include "base/commandlineflags.h"
#include "base/google.h"
#include "base/logging.h"
#include "file/base/file.h"
#include "strings/strutil.h"
#include "testing/base/public/googletest.h"
#include "testing/base/public/gunit.h"
#include "security/streamhtmlparser/htmlparser_cpp.h"

namespace security_streamhtmlparser {

class HtmlparserCppTest : public testing::Test {
 protected:

  typedef hash_map<string, HtmlParser *> ContextMap;

  // Structure that stores the mapping between an id and a name.
  struct IdNameMap {
    int id;
    const char *name;
  };

  // Mapping between the enum and the string representation of the state.
  static const struct IdNameMap kStateMap[];

  // Mapping between the enum and the string representation of the javascript
  // state.
  static const struct IdNameMap kJavascriptStateMap[];

  // Mapping between the enum and the string representation of the attribute
  // type.
  static const struct IdNameMap kAttributeTypeMap[];

  // Mapping between the enum and the string representation of the reset mode.
  static const struct IdNameMap kResetModeMap[];

  // Maximum file size limit.
  static const int kMaxFileSize;

  // String that marks the start of an annotation.
  static const char kDirectiveBegin[];

  // String that marks the end of an annotation.
  static const char kDirectiveEnd[];

  // Count the number of lines in a string.
  static int CountLines(const string &str);

  // Converts a string to a boolean.
  static bool StringToBool(const string &value);

  // Returns the name of the corresponding enum_id by consulting an array of
  // type IdNameMap.
  const char *IdToName(const struct IdNameMap *list, int enum_id);

  // Returns the enum_id of the correspondent name by consulting an array of
  // type IdNameMap.
  int NameToId(const struct IdNameMap *list, const string &name);

  // Reads the filename of an annotated html file and validates the
  // annotations against the html parser state.
  void ValidateFile(string filename);

  // Validate an annotation string against the current parser state.
  void ProcessAnnotation(const string &dir);

  // Validate the parser state against the provided state.
  void ValidateState(const string &tag);

  // Validate the parser tag name against the provided tag name.
  void ValidateTag(const string &tag);

  // Validate the parser attribute name against the provided attribute name.
  void ValidateAttribute(const string &attr);

  // Validate the parser attribute value contents against the provided string.
  void ValidateValue(const string &contents);

  // Validate the parser attribute type against the provided attribute type.
  void ValidateAttributeType(const string &attr);

  // Validate the parser attribute quoted state against the provided
  // boolean.
  void ValidateAttributeQuoted(const string &quoted);

  // Validates the parser in javascript state against the provided boolean.
  void ValidateInJavascript(const string &quoted);

  // Validate the current parser javascript quoted state against the provided
  // boolean.
  void ValidateJavascriptQuoted(const string &quoted);

// Validate the javascript parser state against the provided state.
  void ValidateJavascriptState(const string &expected_state);

  // Validate the current parser value index against the provided index.
  void ValidateValueIndex(const string &value_index);

  void SetUp() {
    parser_.Reset();
  }

  void TearDown() {
    // Delete all parser instances from the context map
    for (ContextMap::iterator iter = contextMap.begin();
        iter != contextMap.end(); ++iter) {
      delete iter->second;
    }
    contextMap.clear();
  }

  // Current line number
  int line_number_;

  // Map containing the registers where the parser context is saved.
  ContextMap contextMap;

  // Parser instance
  HtmlParser parser_;
};

const int HtmlparserCppTest::kMaxFileSize = 1000000;

const char HtmlparserCppTest::kDirectiveBegin[] = "<?state";
const char HtmlparserCppTest::kDirectiveEnd[] = "?>";

const struct HtmlparserCppTest::IdNameMap
             HtmlparserCppTest::kStateMap[] = {
  { HtmlParser::STATE_TEXT,    "text" },
  { HtmlParser::STATE_TAG,     "tag" },
  { HtmlParser::STATE_ATTR,    "attr" },
  { HtmlParser::STATE_VALUE,   "value" },
  { HtmlParser::STATE_COMMENT, "comment" },
  { HtmlParser::STATE_ERROR,   "error" },
  { 0, NULL }
};

const struct HtmlparserCppTest::IdNameMap
             HtmlparserCppTest::kAttributeTypeMap[] = {
  { HtmlParser::ATTR_NONE,    "none" },
  { HtmlParser::ATTR_REGULAR, "regular" },
  { HtmlParser::ATTR_URI,     "uri" },
  { HtmlParser::ATTR_JS,      "js" },
  { HtmlParser::ATTR_STYLE,   "style" },
  { 0, NULL }
};

const struct HtmlparserCppTest::IdNameMap
             HtmlparserCppTest::kJavascriptStateMap[] = {
  { JavascriptParser::STATE_TEXT,    "text" },
  { JavascriptParser::STATE_Q,       "q" },
  { JavascriptParser::STATE_DQ,      "dq" },
  { JavascriptParser::STATE_REGEXP,  "regexp" },
  { JavascriptParser::STATE_COMMENT, "comment" },
  { 0, NULL }
};

const struct HtmlparserCppTest::IdNameMap
             HtmlparserCppTest::kResetModeMap[] = {
  { HtmlParser::MODE_HTML,    "html" },
  { HtmlParser::MODE_JS,      "js" },
  { 0, NULL }
};


// Count the number of lines in a string.
int HtmlparserCppTest::CountLines(const string &str) {
  return strcount(str, '\n');
}

// Converts a string to a boolean.
bool HtmlparserCppTest::StringToBool(const string &value) {
  string lowercase(value);
  LowerString(&lowercase);
  if (lowercase == "true") {
    return true;
  } else if (lowercase == "false") {
    return false;
  } else {
    CHECK("Unknown boolean value" == NULL);
  }
}

// Returns the name of the corresponding enum_id by consulting an array of
// type IdNameMap.
const char *HtmlparserCppTest::IdToName(const struct IdNameMap *list,
                                        int enum_id) {
  CHECK(list != NULL);
  while (list->name) {
    if (enum_id == list->id) {
      return list->name;
    }
    list++;
  }
  CHECK("Unknown id" != NULL);
  return NULL;  // Unreachable.
}

// Returns the enum_id of the correspondent name by consulting an array of
// type IdNameMap.
int HtmlparserCppTest::NameToId(const struct IdNameMap *list,
                                const string &name) {
  CHECK(list != NULL);
  while (list->name) {
    if (name.compare(list->name) == 0) {
      return list->id;
    }
    list++;
  }
  CHECK("Unknown name" != NULL);
  return -1;  // Unreachable.
}

// Validate the parser state against the provided state.
void HtmlparserCppTest::ValidateState(const string &expected_state) {
  const char* parsed_state = IdToName(kStateMap, parser_.state());
  EXPECT_TRUE(parsed_state != NULL);
  EXPECT_TRUE(!expected_state.empty());
  EXPECT_EQ(expected_state, string(parsed_state))
  << "Unexpected state at line " << line_number_;
}

// Validate the parser tag name against the provided tag name.
void HtmlparserCppTest::ValidateTag(const string &expected_tag) {
  EXPECT_TRUE(parser_.tag() != NULL);
  EXPECT_TRUE(expected_tag == parser_.tag())
  << "Unexpected attr tag name at line " << line_number_;
}

// Validate the parser attribute name against the provided attribute name.
void HtmlparserCppTest::ValidateAttribute(const string &expected_attr) {
  EXPECT_TRUE(parser_.attribute() != NULL);
  EXPECT_TRUE(expected_attr == parser_.attribute())
  << "Unexpected attr name value at line " << line_number_;
}

// Validate the parser attribute value contents against the provided string.
void HtmlparserCppTest::ValidateValue(const string &expected_value) {
  EXPECT_TRUE(parser_.value() != NULL);
  const string parsed_state(parser_.value());
  EXPECT_EQ(expected_value, parsed_state)
  << "Unexpected value at line " << line_number_;
}

// Validate the parser attribute type against the provided attribute type.
void HtmlparserCppTest::ValidateAttributeType(
    const string &expected_attr_type) {
  const char *parsed_attr_type = IdToName(kAttributeTypeMap,
                                          parser_.AttributeType());
  EXPECT_TRUE(parsed_attr_type != NULL);
  EXPECT_TRUE(!expected_attr_type.empty());
  EXPECT_EQ(expected_attr_type, string(parsed_attr_type))
  << "Unexpected attr_type value at line " << line_number_;
}

// Validate the parser attribute quoted state against the provided
// boolean.
void HtmlparserCppTest::ValidateAttributeQuoted(
    const string &expected_attr_quoted) {
  bool attr_quoted_bool = StringToBool(expected_attr_quoted);
  EXPECT_EQ(attr_quoted_bool, parser_.IsAttributeQuoted())
  << "Unexpected attr_quoted value at line " << line_number_;
}

// Validates the parser in javascript state against the provided boolean.
void HtmlparserCppTest::ValidateInJavascript(const string &expected_in_js) {
  bool in_js_bool = StringToBool(expected_in_js);
  EXPECT_EQ(in_js_bool, parser_.InJavascript())
  << "Unexpected in_js value at line " << line_number_;
}

// Validate the current parser javascript quoted state against the provided
// boolean.
void HtmlparserCppTest::ValidateJavascriptQuoted(
    const string &expected_js_quoted) {
  bool js_quoted_bool = StringToBool(expected_js_quoted);
  EXPECT_EQ(js_quoted_bool, parser_.IsJavascriptQuoted())
  << "Unexpected js_quoted value at line " << line_number_;
}

// Validate the javascript parser state against the provided state.
void HtmlparserCppTest::ValidateJavascriptState(const string &expected_state) {
  const char* parsed_state = IdToName(kJavascriptStateMap,
                                      parser_.javascript_state());
  EXPECT_TRUE(parsed_state != NULL);
  EXPECT_TRUE(!expected_state.empty());
  EXPECT_EQ(expected_state, string(parsed_state))
  << "Unexpected javascript state at line " << line_number_;
}

// Validate the current parser value index against the provided index.
void HtmlparserCppTest::ValidateValueIndex(const string &expected_value_index) {
  int index;
  CHECK(safe_strto32(expected_value_index, &index));
  EXPECT_EQ(index, parser_.ValueIndex())
  << "Unexpected value_index value at line " << line_number_;
}

// Validate an annotation string against the current parser state.
//
// Split the annotation into a list of key value pairs and call the appropriate
// handler for each pair.
void HtmlparserCppTest::ProcessAnnotation(const string &annotation) {
  vector< pair< string, string > > pairs;
  SplitStringIntoKeyValuePairs(annotation, "=", ",", &pairs);

  vector< pair< string, string > >::iterator iter;

  iter = pairs.begin();
  for (iter = pairs.begin(); iter != pairs.end(); ++iter) {
    StripWhiteSpace(&iter->first);
    StripWhiteSpace(&iter->second);

    if (iter->first.compare("state") == 0) {
      ValidateState(iter->second);
    } else if (iter->first.compare("tag") == 0) {
      ValidateTag(iter->second);
    } else if (iter->first.compare("attr") == 0) {
      ValidateAttribute(iter->second);
    } else if (iter->first.compare("value") == 0) {
      ValidateValue(iter->second);
    } else if (iter->first.compare("attr_type") == 0) {
      ValidateAttributeType(iter->second);
    } else if (iter->first.compare("attr_quoted") == 0) {
      ValidateAttributeQuoted(iter->second);
    } else if (iter->first.compare("in_js") == 0) {
      ValidateInJavascript(iter->second);
    } else if (iter->first.compare("js_quoted") == 0) {
      ValidateJavascriptQuoted(iter->second);
    } else if (iter->first.compare("js_state") == 0) {
      ValidateJavascriptState(iter->second);
    } else if (iter->first.compare("value_index") == 0) {
      ValidateValueIndex(iter->second);
    } else if (iter->first.compare("save_context") == 0) {
      if (contextMap.find(iter->second) == contextMap.end()) {
        contextMap[iter->second] = new HtmlParser();
      }
      contextMap[iter->second]->CopyFrom(&parser_);
    } else if (iter->first.compare("load_context") == 0) {
      CHECK(contextMap.find(iter->second) != contextMap.end());
      parser_.CopyFrom(contextMap[iter->second]);
    } else if (iter->first.compare("reset") == 0) {
      if (StringToBool(iter->second)) {
        parser_.Reset();
      }
    } else if (iter->first.compare("reset_mode") == 0) {
      HtmlParser::Mode mode =
           static_cast<HtmlParser::Mode>(NameToId(kResetModeMap, iter->second));
      parser_.ResetMode(mode);
    } else if (iter->first.compare("insert_text") == 0) {
      if (StringToBool(iter->second)) {
        parser_.InsertText();
      }
    } else {
      FAIL() << "Unknown test directive: " << iter->first;
    }
  }
}

// Validates an html annotated file against the parser state.
//
// It iterates over the html file splitting it into html blocks and annotation
// blocks. It sends the html block to the parser and uses the annotation block
// to validate the parser state.
void HtmlparserCppTest::ValidateFile(string filename) {
  string dir = File::JoinPath(FLAGS_test_srcdir,
                              "google3/security/streamhtmlparser/testdata/");
  string fullpath = File::JoinPath(dir, filename);
  File* file = File::OpenOrDie(fullpath, "r");

  string buffer;
  file->ReadToString(&buffer, kMaxFileSize);

  CHECK(file->Close());

  // Current line count.
  line_number_ = 0;

  // Start of the current html block.
  size_t start_html = 0;

  // Start of the next annotation.
  size_t start_annotation = buffer.find(kDirectiveBegin, 0);

  // Ending of the current annotation.
  size_t end_annotation = buffer.find(kDirectiveEnd, start_annotation);

  while (start_annotation != string::npos) {
    string html_block(buffer, start_html, start_annotation - start_html);
    parser_.Parse(html_block);
    line_number_ += CountLines(html_block);

    start_annotation += strlen(kDirectiveBegin);

    string annotation_block(buffer, start_annotation,
                            end_annotation - start_annotation);
    ProcessAnnotation(annotation_block);
    line_number_ += CountLines(annotation_block);

    start_html = end_annotation + strlen(kDirectiveEnd);
    start_annotation = buffer.find(kDirectiveBegin, start_html);
    end_annotation = buffer.find(kDirectiveEnd, start_annotation);

    // Check for unclosed annotation.
    CHECK(!(start_annotation != string::npos &&
            end_annotation == string::npos));
  }
}

TEST_F(HtmlparserCppTest, SimpleHtml) {
  this->ValidateFile("simple.html");
}

TEST_F(HtmlparserCppTest, Comments) {
  this->ValidateFile("comments.html");
}

TEST_F(HtmlparserCppTest, JavascriptBlock) {
  this->ValidateFile("javascript_block.html");
}

TEST_F(HtmlparserCppTest, JavascriptAttribute) {
  this->ValidateFile("javascript_attribute.html");
}

TEST_F(HtmlparserCppTest, JavascriptRegExp) {
  this->ValidateFile("javascript_regexp.html");
}

TEST_F(HtmlparserCppTest, Tags) {
  this->ValidateFile("tags.html");
}

TEST_F(HtmlparserCppTest, Context) {
  this->ValidateFile("context.html");
}

TEST_F(HtmlparserCppTest, Reset) {
  this->ValidateFile("reset.html");
}

TEST_F(HtmlparserCppTest, CData) {
  this->ValidateFile("cdata.html");
}

TEST_F(HtmlparserCppTest, Error) {
  HtmlParser html;

  ASSERT_EQ(html.Parse("<a href='http://www.google.com' ''>\n"),
            HtmlParser::STATE_ERROR);
}

}  // namespace security_streamhtmlparser

int main(int argc, char **argv) {
  FLAGS_logtostderr = true;
  InitGoogle(argv[0], &argc, &argv, true);

  File::Init();

  return RUN_ALL_TESTS();
}
