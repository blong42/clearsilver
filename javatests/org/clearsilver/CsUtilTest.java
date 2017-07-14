// Copyright 2008 Google Inc. All Rights Reserved.

package org.clearsilver;

import junit.framework.TestCase;

import java.util.List;

/**
 * Unittests for CSUtil.
 *
 * @author Sergio Marti (smarti@google.com)
 */
public class CsUtilTest extends TestCase {

  public void testGetLoadPaths_noLoadPaths() {
    ClearsilverFactory csFactory = FactoryLoader.getClearsilverFactory();
    HDF hdf = csFactory.newHdf();

    List<String> loadPaths = CSUtil.getLoadPaths(hdf, true);
    assertTrue(loadPaths.isEmpty());

    try {
      loadPaths = CSUtil.getLoadPaths(hdf, false);
      fail("NullPointerException expected");
    } catch (NullPointerException e) {
      // expected.
    }

    hdf.setValue(CSUtil.HDF_LOADPATHS, "1");

    loadPaths = CSUtil.getLoadPaths(hdf, true);
    assertTrue(loadPaths.isEmpty());
    loadPaths = CSUtil.getLoadPaths(hdf, false);
    assertTrue(loadPaths.isEmpty());
  }

  public void testGetLoadPaths_withLoadPaths() {
    ClearsilverFactory csFactory = FactoryLoader.getClearsilverFactory();
    HDF hdf = csFactory.newHdf();
    hdf.setValue(CSUtil.HDF_LOADPATHS + ".0", "/dir0");

    List<String> loadPaths = CSUtil.getLoadPaths(hdf);
    assertEquals(1, loadPaths.size());
    assertEquals("/dir0", loadPaths.get(0));

    hdf.setValue(CSUtil.HDF_LOADPATHS + ".1", "/dir1");
    hdf.setValue(CSUtil.HDF_LOADPATHS + ".2", "/dir2");
    hdf.setValue(CSUtil.HDF_LOADPATHS + ".3", "/dir3");
    hdf.setValue(CSUtil.HDF_LOADPATHS + ".4", "/dir4");

    loadPaths = CSUtil.getLoadPaths(hdf, false);
    assertEquals(5, loadPaths.size());
    assertEquals("/dir0", loadPaths.get(0));
    assertEquals("/dir2", loadPaths.get(2));
    assertEquals("/dir4", loadPaths.get(4));

    loadPaths = CSUtil.getLoadPaths(hdf, true);
    assertEquals(5, loadPaths.size());
    assertEquals("/dir0", loadPaths.get(0));
    assertEquals("/dir2", loadPaths.get(2));
    assertEquals("/dir4", loadPaths.get(4));
  }

}
