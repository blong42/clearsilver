package org.clearsilver;

/**
 * A factory for constructing new CS and HDF objects.  Allows applications to
 * provide subclasses of HDF or CS to be used by the Java Clearsilver
 * templating system.
 *
 * @author smarti@google.com (Sergio Marti)
 */
public interface ClearsilverFactory {

  /**
   * Create a new CS object.
   * @param hdf the HDF object to use in constructing the CS object.
   * @return a new CS object
   */
  public CS newCs(HDF hdf);

  /**
   * Create a new CS object.
   * @param hdf the HDF object to use in constructing the CS object.
   * @param globalHdf the global HDF object to use in constructing the
   * CS object.
   * @return a new CS object
   */
  public CS newCs(HDF hdf, HDF globalHdf);

  /**
   * Create a new HDF object.
   * @return a new HDF object
   */
  public HDF newHdf();
}
