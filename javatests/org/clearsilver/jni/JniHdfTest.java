// Copyright 2008 Google Inc. All Rights Reserved.

package org.clearsilver.jni;

import org.clearsilver.HdfTest;
import org.clearsilver.ClearsilverFactory;

/**
 * Tests specifically for JniHdf.
 *
 * @author Sergio Marti (smarti@google.com)
 */
public class JniHdfTest extends HdfTest {

  @Override
  protected ClearsilverFactory newClearsilverFactory() {
    return new JniClearsilverFactory();
  }
}
