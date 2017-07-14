// Copyright 2008 Google Inc. All Rights Reserved.

package org.clearsilver.jni;

import org.clearsilver.ClearsilverFactory;
import org.clearsilver.CsTest;

/**
 * Unittests specifically for JniCs
 *
 * @author Sergio Marti (smarti@google.com)
 */
public class JniCsTest extends CsTest {

  @Override
  protected ClearsilverFactory newClearsilverFactory() {
    return new JniClearsilverFactory();
  }
}
