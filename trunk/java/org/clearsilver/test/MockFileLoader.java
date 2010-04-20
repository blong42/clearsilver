// Copyright 2008 Google Inc. All Rights Reserved.

package org.clearsilver.test;

import org.clearsilver.CSFileLoader;
import org.clearsilver.HDF;

import java.util.Map;
import java.util.HashMap;
import java.io.IOException;
import java.io.FileNotFoundException;

/**
 * Test utility class that can be used as a simple filesystem replacement.
*
* @author Sergio Marti (smarti@google.com)
*/
public class MockFileLoader implements CSFileLoader {
  private final Map<String, String> fileMap = new HashMap<String, String>();

  public String load(HDF hdf, String filename) throws IOException {
    if (!fileMap.containsKey(filename)) {
      throw new FileNotFoundException("Can't find test file " + filename);
    }
    return fileMap.get(filename);
  }

  public void addMapping(String filename, String content) {
    fileMap.put(filename, content);
  }
}
