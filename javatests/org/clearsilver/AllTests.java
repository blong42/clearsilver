package org.clearsilver;

import junit.framework.Test;
import junit.framework.TestSuite;

import org.clearsilver.jni.JniCsTest;
import org.clearsilver.jni.JniHdfTest;

import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

public final class AllTests extends TestSuite {

  /**
   * Used in main to run all tests.
   *
   * @return tests
   * @throws IOException
   */
  public static Test suite() throws Exception {

    // disable logging except for severe
    Logger.getLogger("org.clearsilver").setLevel(Level.OFF);

    TestSuite suite = new TestSuite("org.clearsilver.AllTests");
    suite.addTestSuite(CsTest.class);
    suite.addTestSuite(CsUtilTest.class);
    suite.addTestSuite(DelegatedCsTest.class);
    suite.addTestSuite(DelegatedHdfTest.class);
    suite.addTestSuite(HdfTest.class);

    // JNI tests
    suite.addTestSuite(JniCsTest.class);
    suite.addTestSuite(JniHdfTest.class);

    return suite;
  }

  /**
   * Runs all Unit tests
   * @param args
   */
  public static void main(String[] args) throws Exception {
    junit.textui.TestRunner.run(suite());
  }

}
