package org.clearsilver;

import java.io.*;
import java.util.*;

public class CS {
  public int csptr;
  
  static { 
    try {
      System.loadLibrary("clearsilver-jni");
    } catch ( UnsatisfiedLinkError e ) {
      System.out.println("Could not load library 'clearsilver-jni'");
      System.exit(1);
    }
  }
  
  public CS(HDF ho) {
    csptr = _init(ho.hdfptr);
  }

  public void finalize() {
    _dealloc(csptr);
  }

  public void parseFile(String filename) {
    _parseFile(csptr,filename);
  }

  public void parseStr(String content) {
    _parseStr(csptr,content);
  }

  public String render() {
    return _render(csptr);
  }
  
  private native int    _init(int ptr);
  private native void   _dealloc(int ptr);
  private native void   _parseFile(int ptr,String filename);
  private native void   _parseStr(int ptr, String content);
  private native String _render(int ptr);
};
