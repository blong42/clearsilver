// Copyright 2008 Google Inc. All Rights Reserved.

package org.clearsilver;

import junit.framework.TestCase;

import java.io.File;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.Map;

/**
 * Common temp file creation and deletion for other tests.
 *
 * @author Sergio Marti (smarti@google.com)
 */
public abstract class ClearsilverTestCase extends TestCase {

  private static final String TMP_DIR_PREFIX;
  protected static final Map<String, String> files = new HashMap<String, String>();

  static {
    TMP_DIR_PREFIX = System.getProperty("java.io.tmpdir", "/tmp") + 
        "/org_clearsilver_test_";

    files.put("Simple.hdf", "foo = Hello");
    files.put("Simple.cs", "Say <?cs var:foo ?>");
    files.put("Include.hdf", "#include Simple.hdf\nbar = you");
    files.put("MissingInclude.hdf", "#include Missing.hdf\nbar = you");
    files.put("SoftInclude.cs", "Load <?cs include:\"Simple.cs\" ?>");
    files.put("MissingSoftInclude.cs", "Load <?cs include:\"Missing.cs\" ?>");
    files.put("HardInclude.cs", "Load <?cs include!\"Simple.cs\" ?>");
    files.put("MissingHardInclude.cs", "Load <?cs include!\"Missing.cs\" ?>");
  }

  protected ClearsilverFactory csFactory;
  protected String directoryName;

  protected ClearsilverFactory newClearsilverFactory() {
    return FactoryLoader.getClearsilverFactory();
  }

  @Override
  protected void setUp() throws Exception {
    super.setUp();

    csFactory = newClearsilverFactory();

    long time = System.currentTimeMillis();
    directoryName = TMP_DIR_PREFIX + Long.toString(time);
    boolean success = false;
    try {
      File dir = new File(directoryName);
      if (!dir.mkdir()) {
        throw new IllegalStateException("Unable to create tmp directory " +
            directoryName);
      }
      for (Map.Entry<String, String> file : files.entrySet()) {
        File f = new File(directoryName, file.getKey());
        if (!f.createNewFile()) {
          throw new IllegalStateException("Unable to create tmp file " +
              file.getKey());
        }
        PrintWriter writer = new PrintWriter(f);
        writer.print(file.getValue());
        writer.close();
      }
      success = true;
    } finally {
      if (!success) {
        cleanUpFiles();
      }
    }
  }

  @Override
  protected void tearDown() throws Exception {
    cleanUpFiles();
    super.tearDown();
  }

  private void cleanUpFiles() {
    File dir = new File(directoryName);
    if (!dir.exists() || !dir.isDirectory()) {
      return;
    }
    for (File f : dir.listFiles()) {
      f.delete();
    }
    dir.delete();
  }

}
