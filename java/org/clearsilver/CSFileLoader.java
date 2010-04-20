package org.clearsilver;

import java.io.IOException;

/**
 * Interface for CS file hook
 *
 * @author smarti@google.com (Sergio Marti)
 */
public interface CSFileLoader {

  /**
   * Callback method that is expected to return the contents of the specified
   * file as a string.
   * @param hdf the HDF structure associated with HDF or CS object making the
   * callback.
   * @param filename the name of the file that should be loaded.
   * @return a string containing the contents of the file.
   */
  public String load(HDF hdf, String filename) throws IOException;

}
