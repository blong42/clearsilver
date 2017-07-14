// Copyright 2008 Google Inc. All Rights Reserved.

package org.clearsilver;

import org.clearsilver.test.FSFileLoader;

import java.io.FileNotFoundException;
import java.io.IOException;

/**
 * Testing HDF API.
 *
 * @author Sergio Marti (smarti@google.com)
 */
public class HdfTest extends ClearsilverTestCase {

  protected HDF hdf;
  private CSFileLoader fileLoader;

  @Override
  protected void setUp() throws Exception {
    super.setUp();
    hdf = csFactory.newHdf();
    hdf.setValue(CSUtil.HDF_LOADPATHS + ".0", directoryName);
    fileLoader = new FSFileLoader();
  }

  private void verifyReadFile_fileExists(HDF hdf) throws IOException {
    hdf.setFileLoader(fileLoader);
    assertEquals(null, hdf.getValue("foo", null));
    hdf.readFile("Simple.hdf");
    assertEquals("Hello", hdf.getValue("foo", null));
  }


  public void testReadFile_fileExists_withCallback() throws IOException {
    hdf.setFileLoader(fileLoader);
    verifyReadFile_fileExists(hdf);
  }

  public void testReadFile_fileExists_noCallback() throws IOException {
    verifyReadFile_fileExists(hdf);
  }

  private void verifyReadFile_fileNotExists(HDF hdf) throws IOException {
    assertEquals(null, hdf.getValue("foo", null));
    try {
      hdf.readFile("Missing.hdf");
      fail("Expected FileNotFoundException");
    } catch (FileNotFoundException e) {
      // Expected.
    }
    assertEquals(null, hdf.getValue("foo", null));
  }

  public void testReadFile_fileNotExists_withCallback() throws IOException {
    hdf.setFileLoader(fileLoader);
    verifyReadFile_fileNotExists(hdf);
  }

  public void testReadFile_fileNotExists_noCallback() throws IOException {
    verifyReadFile_fileNotExists(hdf);
  }


  private void verifyReadFile_include(HDF hdf) throws IOException {
    assertEquals(null, hdf.getValue("foo", null));
    assertEquals(null, hdf.getValue("bar", null));
    hdf.readFile("Include.hdf");
    assertEquals("Hello", hdf.getValue("foo", null));
    assertEquals("you", hdf.getValue("bar", null));
  }

  public void testReadFile_include_withCallback() throws IOException {
    hdf.setFileLoader(fileLoader);
    verifyReadFile_include(hdf);
  }

  public void testReadFile_include_noCallback() throws IOException {
    verifyReadFile_include(hdf);
  }

  private void verifyReadFile_missingInclude(HDF hdf) throws IOException {
    assertEquals(null, hdf.getValue("foo", null));
    assertEquals(null, hdf.getValue("bar", null));
    try {
      hdf.readFile("MissingInclude.hdf");
      fail("Expected FileNotFoundException");
    } catch (FileNotFoundException e) {
      // Expected
    }
    assertEquals(null, hdf.getValue("foo", null));
    assertEquals(null, hdf.getValue("bar", null));
  }

  public void testReadFile_missingInclude_withCallback() throws IOException {
    hdf.setFileLoader(fileLoader);
    verifyReadFile_missingInclude(hdf);
  }

  public void testReadFile_missingInclude_noCallback() throws IOException {
    verifyReadFile_missingInclude(hdf);
  }

  public void testSetSymLink_sourceExists() {
    hdf.setValue("foo.bar", "1");
    hdf.setSymLink("link", "foo.bar");

    assertEquals("1", hdf.getValue("link", "INVALID"));
    hdf.setValue("foo.bar", "2");
    assertEquals("2", hdf.getValue("link", "INVALID"));
  }

  public void testBelongsToSameRoot() {
    assertTrue(hdf.belongsToSameRoot(hdf));

    hdf.setValue("foo.bar", "1");
    assertTrue(hdf.belongsToSameRoot(hdf.getObj("foo")));
    assertTrue(hdf.belongsToSameRoot(hdf.getOrCreateObj("bar")));

    HDF anotherHdf = csFactory.newHdf();
    assertFalse(hdf.belongsToSameRoot(anotherHdf));
    assertFalse(hdf.belongsToSameRoot(anotherHdf.getOrCreateObj("foo")));
    assertFalse(hdf.getObj("foo").belongsToSameRoot(anotherHdf));
    assertFalse(hdf.getObj("foo").belongsToSameRoot(
        anotherHdf.getOrCreateObj("foo")));
  }

  public void testGettingSettingEmptyStringPath() {
    hdf.setValue("foo", "1");
    assertEquals("1", hdf.getValue("foo", "ERROR"));
    HDF child = hdf.getObj("foo");
    // Asking for value of empty string path is equivalent to asking for current
    // node value.
    assertEquals("1", child.getValue("", "ERROR"));
    assertEquals("foo", child.objName());
    assertEquals("1", child.objValue());
    // Setting value of empty string path is equivalent to setting current node
    // value.
    child.setValue("", "2");
    assertEquals("2", child.objValue());
    assertEquals("2", hdf.getValue("foo", "ERROR"));
  }

  public void testDefaultIntermediateNodeValues() {
    // Setting a value down several new nodes results in no values for
    // intermediate nodes.
    hdf.setValue("A.B.C", "3");
    assertEquals("ERROR", hdf.getValue("A", "ERROR"));
    assertEquals("ERROR", hdf.getValue("A.B", "ERROR"));
    assertEquals("3", hdf.getValue("A.B.C", "ERROR"));
    HDF child = hdf.getOrCreateObj("A");
    assertEquals("ERROR", child.getValue("", "ERROR"));
  }

  public void testGetOrCreateObjDefaultValue() {
    // getObj does not create a node.
    HDF child = hdf.getObj("foo");
    assertEquals(null, child);
    // getOrCreateObject creates a node and sets the value to empty string.
    child = hdf.getOrCreateObj("foo");
    assertEquals("", child.getValue("", "ERROR"));
  }
}
