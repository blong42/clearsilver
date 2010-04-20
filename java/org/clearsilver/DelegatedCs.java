// Copyright 2008 Google Inc. All Rights Reserved.

package org.clearsilver;

import java.io.IOException;

/**
 * Utility class that delegates all methods of an CS object. Made to
 * facilitate the transition to CS being an interface and thus not
 * extensible in the same way as it was.
 * <p>
 * This class, and its subclasses must take care to wrap or unwrap HDF and CS
 * objects as they are passed through from the callers to the delegate object.
 *
 * @author Sergio Marti (smarti@google.com)
 */
public abstract class DelegatedCs implements CS {
  private final CS cs;

  public DelegatedCs(CS cs) {
    // Give it an empty HDF. We aren't going to be using the super object anyways.
    this.cs = cs;
  }

  public CS getCs() {
    return cs;
  }

  /**
   * Method subclasses are required to override with a method that returns a
   * new DelegatedHdf object that wraps the specified HDF object.
   *
   * @param hdf an HDF object that should be wrapped in a new DelegatedHdf
   * object of the same type as this current object.
   * @return an object that is a subclass of DelegatedHdf and which wraps the
   * given HDF object.
   */
  protected abstract DelegatedHdf newDelegatedHdf(HDF hdf);

  public void setGlobalHDF(HDF global) {
    if (global != null && global instanceof DelegatedHdf) {
      global = ((DelegatedHdf)global).getHdf();
    }
    getCs().setGlobalHDF(global);
  }

  public HDF getGlobalHDF() {
    HDF hdf =  getCs().getGlobalHDF();
    return hdf != null ? newDelegatedHdf(hdf) : null;
  }

  public void close() {
    getCs().close();
  }

  public void parseFile(String filename) throws IOException {
    getCs().parseFile(filename);
  }
  public void parseStr(String content) {
    getCs().parseStr(content);
  }

  public String render() {
    return getCs().render();
  }

  public CSFileLoader getFileLoader() {
    return getCs().getFileLoader();
  }

  public void setFileLoader(CSFileLoader fileLoader) {
    getCs().setFileLoader(fileLoader);
  }

}
