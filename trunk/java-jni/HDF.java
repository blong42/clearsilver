package org.clearsilver;

import java.io.*;
import java.util.*;

public class HDF {
    public int hdfptr;
    
    static { 
	try {
	    System.loadLibrary("jni_neo_cgi");
	} catch ( UnsatisfiedLinkError e ) {
	    System.out.println("Could not load libjni_neo_cgi.so");
	    System.exit(1);
	}
    }
    
    public HDF() {
	hdfptr = _init();
    }
    public void finalize() {
	_dealloc(hdfptr);
    }

    public int getIntValue(String hdfname, int default_value) {
	return _getIntValue(hdfptr,hdfname,default_value);
    }

    public String getValue(String hdfname, String default_value) {
	return _getValue(hdfptr,hdfname,default_value);
    }

    public void setValue(String hdfname, String value) {
	_setValue(hdfptr,hdfname,value);
    }
    public String dump() {
	return _dump(hdfptr);
    }

    private native int    _init();
    private native void   _dealloc(int ptr);
    private native int    _getIntValue(int ptr, String hdfname,int default_value);
    private native String _getValue(int ptr, String hdfname, String default_value);
    private native void   _setValue(int ptr, String hdfname, String hdf_value);
    private native String _dump(int ptr);
};
