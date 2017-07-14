// Copyright 2008 Google Inc. All Rights Reserved.

package org.clearsilver;

import junit.framework.TestCase;

import org.easymock.EasyMock;
import org.easymock.IMocksControl;

/**
 * Unittests for DelegatedCs
 *
 * @author Sergio Marti (smarti@google.com)
 */
public class DelegatedCsTest extends TestCase {

  private IMocksControl mockControl;
  private CS mockCs;

  static class TestDelegatedCs extends DelegatedCs {

    public TestDelegatedCs(CS cs) {
      super(cs);
    }

    protected DelegatedHdf newDelegatedHdf(HDF hdf) {
      return new DelegatedHdfTest.TestDelegatedHdf(hdf);
    }
  }

  protected void setUp() throws Exception {
    super.setUp();
    mockControl = EasyMock.createControl();
    mockCs = mockControl.createMock(CS.class);
  }

  public void testConstructor() {
    mockCs.close();
    mockControl.replay();
    DelegatedCs cs = new TestDelegatedCs(mockCs);
    cs.close();
    mockControl.verify();
  }

  public void testSetGlobalHDF_notWrapped() {
    HDF mockHdf = mockControl.createMock(HDF.class);
    mockCs.setGlobalHDF(EasyMock.eq(mockHdf));
    EasyMock.expect(mockCs.getGlobalHDF()).andReturn(mockHdf);
    mockControl.replay();
    DelegatedCs cs = new TestDelegatedCs(mockCs);
    cs.setGlobalHDF(mockHdf);
    HDF globalHdf = cs.getGlobalHDF();
    assertTrue(globalHdf instanceof DelegatedHdf);
    assertSame(mockHdf, ((DelegatedHdf)globalHdf).getHdf());
    mockControl.verify();
  }

  public void testSetGlobalHDF_wrapped() {
    HDF mockHdf = mockControl.createMock(HDF.class);
    HDF globalHdf = new DelegatedHdfTest.TestDelegatedHdf(mockHdf);
    mockCs.setGlobalHDF(EasyMock.eq(mockHdf));
    EasyMock.expect(mockCs.getGlobalHDF()).andReturn(mockHdf);
    mockControl.replay();
    DelegatedCs cs = new TestDelegatedCs(mockCs);
    cs.setGlobalHDF(globalHdf);
    globalHdf = cs.getGlobalHDF();
    assertTrue(globalHdf instanceof DelegatedHdf);
    assertSame(mockHdf, ((DelegatedHdf)globalHdf).getHdf());
    mockControl.verify();
  }

  public void testSetGlobalHDF_null() {
    mockCs.setGlobalHDF((HDF)EasyMock.isNull());
    EasyMock.expect(mockCs.getGlobalHDF()).andReturn(null);
    mockControl.replay();
    DelegatedCs cs = new TestDelegatedCs(mockCs);
    cs.setGlobalHDF(null);
    assertNull(cs.getGlobalHDF());
    mockControl.verify();
  }

  public void testClose() {
    mockCs.close();
    mockControl.replay();
    DelegatedCs cs = new TestDelegatedCs(mockCs);
    cs.close();
    mockControl.verify();
  }

}
