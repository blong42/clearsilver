// Copyright 2008 Google Inc. All Rights Reserved.

package org.clearsilver;

import org.clearsilver.test.FSFileLoader;

import java.io.FileNotFoundException;
import java.io.IOException;

/**
 * Tests for the CS API
 *
 * @author Sergio Marti (smarti@google.com)
 */
public class CsTest extends ClearsilverTestCase {

  protected HDF hdf;
  protected CS cs;
  protected CSFileLoader fileLoader;

  @Override
  protected void setUp() throws Exception {
    super.setUp();
    hdf = csFactory.newHdf();
    hdf.setValue(CSUtil.HDF_LOADPATHS + ".0", directoryName);
    cs = csFactory.newCs(hdf);
    fileLoader = new FSFileLoader();
  }

  public void testWithArgChildInMacroCalledTwiceFirstWithNonExistentVar() {
    hdf.setValue("A", "WRONG");
    hdf.setValue("A.bar", "RIGHT");

    String template = ""
        + "<?cs def:aMacro(foo) ?>\n"
        + "<?cs with:item = foo.bar ?><?cs var:item ?><?cs /with ?>\n"
        + "<?cs /def ?>\n"
        + "<?cs call:aMacro(unknownVar) ?>\n"
        + "<?cs call:aMacro(A) ?>";

    String expected = ""
        + "\n"
        + "\n"
        + "\n"
        + "\n"
        + "\n"
        + "RIGHT\n";

    cs.parseStr(template);
    assertEquals(expected, cs.render());
  }

  public void testMultiArgMacroScoping() {
    hdf.setValue("foo", "WRONG");

    String template = ""
        + "<?cs def:macro1(arg1, arg2) ?><?cs alt:arg1 ?>RIGHT<?cs /alt ?><?cs /def ?>\n"
        + "<?cs def:macro2(arg1, arg2) ?><?cs call:macro1(arg2, arg1) ?><?cs "
        + "/def ?>\n"
        + "<?cs call:macro2(foo, bar) ?>";

    String expected = ""
        + "\n"
        + "\n"
        + "RIGHT";
    
    cs.parseStr(template);
    assertEquals(expected, cs.render());
  }

  public void testGlobalHdfIsNotModified() {
    HDF globalHdf = csFactory.newHdf();
    globalHdf.setValue("foo", "OLD");
    cs.setGlobalHDF(globalHdf);

    String template = ""
        + "<?cs var:foo ?>\n"
        + "<?cs set:foo = \"NEW\" ?>\n"
        + "<?cs var:foo ?>";

    String expected = ""
        + "OLD\n"
        + "\n"
        + "NEW";
    cs.parseStr(template);
    assertEquals(expected, cs.render());
    assertEquals("OLD", globalHdf.getValue("foo", "UNKNOWN"));
  }


  private void verifyParseFile_csFileExists(CS cs) throws IOException {
    cs.parseFile("Simple.cs");
    assertEquals("Say ", cs.render());
  }

  public void testParseFile_csFileExists_withCallback() throws IOException {
    cs.setFileLoader(fileLoader);
    verifyParseFile_csFileExists(cs);
  }

  public void testParseFile_csFileExists_noCallback() throws IOException {
    verifyParseFile_csFileExists(cs);
  }


  private void verifyParseFile_csHdfFileExists(CS cs) throws IOException {
    hdf.readFile("Simple.hdf");
    cs.parseFile("Simple.cs");
    assertEquals("Say Hello", cs.render());
  }

  public void testParseFile_csHdfFileExists_withCallback() throws IOException {
    hdf.setFileLoader(fileLoader);
    verifyParseFile_csHdfFileExists(cs);
  }

  public void testParseFile_csHdfFileExists_noCallback() throws IOException {
    verifyParseFile_csHdfFileExists(cs);
  }

  private void verifyParseFile_fileNotExists(CS cs) throws Exception {
    try {
      cs.parseFile("Missing.cs");
      fail("Expected FileNotFoundException");
    } catch (FileNotFoundException e) {
      // Expected
    }
  }

  public void testParseFile_fileNotExists_withCallback() throws Exception {
    cs.setFileLoader(fileLoader);
    verifyParseFile_fileNotExists(cs);
  }

  public void testParseFile_fileNotExists_noCallback() throws Exception {
    verifyParseFile_fileNotExists(cs);
  }


  private void verifyParseFile_hardInclude(CS cs) throws Exception {
    cs.parseFile("HardInclude.cs");
    assertEquals("Load Say ", cs.render());
  }

  public void testParseFile_hardInclude_withCallback() throws Exception {
    cs.setFileLoader(fileLoader);
    verifyParseFile_hardInclude(cs);
  }

  public void testParseFile_hardInclude_noCallback() throws Exception {
    verifyParseFile_hardInclude(cs);
  }


  private void verifyParseFile_missingHardInclude(CS cs) throws Exception {
    try {
      cs.parseFile("MissingHardInclude.cs");
      fail("Expected FileNotFoundException");
    } catch (FileNotFoundException e) {
      // Expected
    }
  }

  public void testParseFile_missingHardInclude_withCallback() throws Exception {
    cs.setFileLoader(fileLoader);
    verifyParseFile_missingHardInclude(cs);
  }

  public void testParseFile_missingHardInclude_noCallback() throws Exception {
    verifyParseFile_missingHardInclude(cs);
  }


  private void verifyParseFile_softInclude(CS cs) throws Exception {
    cs.parseFile("SoftInclude.cs");
    assertEquals("Load Say ", cs.render());
  }

  public void testParseFile_softInclude_withCallback() throws Exception {
    cs.setFileLoader(fileLoader);
    verifyParseFile_softInclude(cs);
  }

  public void testParseFile_softInclude_noCallback() throws Exception {
    verifyParseFile_softInclude(cs);
  }


  private void verifyParseFile_missingSoftInclude(CS cs) throws Exception {
    cs.parseFile("MissingSoftInclude.cs");
    assertEquals("Load ", cs.render());
  }

  public void testParseFile_missingSoftInclude_withCallback() throws Exception {
    cs.setFileLoader(fileLoader);
    verifyParseFile_missingSoftInclude(cs);
  }

  public void testParseFile_missingSoftInclude_noCallback() throws Exception {
    verifyParseFile_missingSoftInclude(cs);
  }

  public void testGetAndSetFileLoader() throws Exception {
    assertNull(cs.getFileLoader());

    CSFileLoader fileLoader = new CSFileLoader() {
      public String load(HDF hdf, String filename) throws IOException {
        return null;
      }
    };

    cs.setFileLoader(fileLoader);
    assertEquals(fileLoader, cs.getFileLoader());
  }
}
