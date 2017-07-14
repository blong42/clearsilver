// Copyright 2008 Google Inc. All Rights Reserved.

package org.clearsilver;

import junit.framework.TestCase;

import org.easymock.EasyMock;
import org.easymock.IMocksControl;

/**
 * Unittests for DelegatedHdf
 *
 * @author Sergio Marti (smarti@google.com)
 */
public class DelegatedHdfTest extends TestCase {

  private IMocksControl mockControl;
  private HDF mockHdf;
  private HDF otherMockHdf;

  static class TestDelegatedHdf extends DelegatedHdf {

    public TestDelegatedHdf(HDF hdf) {
      super(hdf);
    }

    protected DelegatedHdf newDelegatedHdf(HDF hdf) {
      return new TestDelegatedHdf(hdf);
    }
  }

  protected void setUp() throws Exception {
    super.setUp();
    mockControl = EasyMock.createControl();
    mockHdf = mockControl.createMock(HDF.class);
    otherMockHdf = mockControl.createMock(HDF.class);
  }

  public void testConstructor() {
    mockHdf.close();
    mockControl.replay();
    DelegatedHdf hdf = new TestDelegatedHdf(mockHdf);
    assertSame(mockHdf, hdf.getHdf());
    hdf.close();
    mockControl.verify();
  }

  public void testGetChild() {
    String path = "foo";
    EasyMock.expect(mockHdf.getChild(EasyMock.eq("foo"))).andReturn(otherMockHdf);
    mockControl.replay();
    DelegatedHdf hdf = new TestDelegatedHdf(mockHdf);
    HDF childHdf = hdf.getChild(path);
    assertTrue(childHdf instanceof DelegatedHdf);
    assertSame(otherMockHdf, ((DelegatedHdf)childHdf).getHdf());
    mockControl.verify();
  }

  public void testGetChild_null() {
    String path = "foo";
    EasyMock.expect(mockHdf.getChild(EasyMock.eq("foo"))).andReturn(null);
    mockControl.replay();
    DelegatedHdf hdf = new TestDelegatedHdf(mockHdf);
    assertNull(hdf.getChild(path));
    mockControl.verify();
  }

  public void testObjNext() {
    EasyMock.expect(mockHdf.objNext()).andReturn(otherMockHdf);
    mockControl.replay();
    DelegatedHdf hdf = new TestDelegatedHdf(mockHdf);
    HDF nextHdf = hdf.objNext();
    assertTrue(nextHdf instanceof DelegatedHdf);
    assertSame(otherMockHdf, ((DelegatedHdf)nextHdf).getHdf());
    mockControl.verify();
  }

  public void testObjNext_null() {
    EasyMock.expect(mockHdf.objNext()).andReturn(null);
    mockControl.replay();
    DelegatedHdf hdf = new TestDelegatedHdf(mockHdf);
    assertNull(hdf.objNext());
    mockControl.verify();
  }

  public void testGetObj() {
    EasyMock.expect(mockHdf.getObj(EasyMock.eq("path"))).andReturn(otherMockHdf);
    mockControl.replay();
    DelegatedHdf hdf = new TestDelegatedHdf(mockHdf);
    HDF objHdf = hdf.getObj("path");
    assertTrue(objHdf instanceof DelegatedHdf);
    assertSame(otherMockHdf, ((DelegatedHdf)objHdf).getHdf());
    mockControl.verify();
  }

  public void testGetObj_null() {
    EasyMock.expect(mockHdf.getObj(EasyMock.eq("path"))).andReturn(null);
    mockControl.replay();
    DelegatedHdf hdf = new TestDelegatedHdf(mockHdf);
    assertNull(hdf.getObj("path"));
    mockControl.verify();
  }

  public void testTripleDelegation() {
    EasyMock.expect(mockHdf.getRootObj()).andReturn(otherMockHdf);
    mockHdf.close();
    otherMockHdf.close();
    mockControl.replay();
    DelegatedHdf hdf1 = new TestDelegatedHdf(mockHdf);
    DelegatedHdf hdf2 = new TestDelegatedHdf(hdf1);
    DelegatedHdf hdf3 = new TestDelegatedHdf(hdf2);
    assertSame(mockHdf, hdf1.getHdf());
    assertSame(hdf1, hdf2.getHdf());
    assertSame(hdf2, hdf3.getHdf());
    HDF rootHdf = hdf3.getRootObj();
    // Unwrap the layers on top of the rootHdf.
    assertTrue(rootHdf instanceof DelegatedHdf);
    rootHdf = ((DelegatedHdf)rootHdf).getHdf();
    assertTrue(rootHdf instanceof DelegatedHdf);
    rootHdf = ((DelegatedHdf)rootHdf).getHdf();
    assertSame(otherMockHdf, ((DelegatedHdf)rootHdf).getHdf());
    hdf3.close();
    rootHdf.close();
    mockControl.verify();
  }

  public void testBelongsToSameRoot() {
    EasyMock.expect(mockHdf.belongsToSameRoot(mockHdf)).andReturn(true).anyTimes();
    EasyMock.expect(mockHdf.belongsToSameRoot(otherMockHdf)).andReturn(false).anyTimes();
    EasyMock.expect(otherMockHdf.belongsToSameRoot(mockHdf)).andReturn(false).anyTimes();
    mockControl.replay();

    DelegatedHdf hdf1 = new TestDelegatedHdf(mockHdf);
    DelegatedHdf hdf1wrapped = new TestDelegatedHdf(hdf1);
    DelegatedHdf hdf2 = new TestDelegatedHdf(mockHdf);
    DelegatedHdf hdf3 = new TestDelegatedHdf(otherMockHdf);

    assertTrue(hdf1.belongsToSameRoot(hdf1));
    assertTrue(hdf1.belongsToSameRoot(hdf1wrapped));
    assertTrue(hdf1.belongsToSameRoot(hdf2));
    assertFalse(hdf1.belongsToSameRoot(hdf3));
    assertFalse(hdf1wrapped.belongsToSameRoot(hdf3));
    assertFalse(hdf2.belongsToSameRoot(hdf3));
    mockControl.verify();
  }

  public void testCopy_notWrapped() {
    String path = "foo";
    mockHdf.copy(EasyMock.eq(path), EasyMock.eq(otherMockHdf));
    mockControl.replay();
    DelegatedHdf hdf = new TestDelegatedHdf(mockHdf);
    hdf.copy(path, otherMockHdf);
    mockControl.verify();
  }

  public void testCopy_wrapped() {
    String path = "foo";
    mockHdf.copy(EasyMock.eq(path), EasyMock.eq(otherMockHdf));
    mockControl.replay();
    DelegatedHdf hdf = new TestDelegatedHdf(mockHdf);
    hdf.copy(path, new TestDelegatedHdf(otherMockHdf));
    mockControl.verify();
  }

  public void testCopy_null() {
    String path = "foo";
    mockHdf.copy(EasyMock.eq(path), (HDF)EasyMock.isNull());
    mockControl.replay();
    DelegatedHdf hdf = new TestDelegatedHdf(mockHdf);
    hdf.copy(path, null);
    mockControl.verify();
  }

  public void testClose() {
    mockHdf.close();
    mockControl.replay();
    DelegatedHdf hdf = new TestDelegatedHdf(mockHdf);
    hdf.close();
    mockControl.verify();
  }


 public void testGetFullyUnwrappedHdf() {
   DelegatedHdf delegatedHdf = new TestDelegatedHdf(mockHdf);
   delegatedHdf = new TestDelegatedHdf(delegatedHdf);
   assertNotSame(mockHdf, delegatedHdf);
   assertSame(mockHdf, DelegatedHdf.getFullyUnwrappedHdf(delegatedHdf));
 }
}
