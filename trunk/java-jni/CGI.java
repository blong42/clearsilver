
import java.io.*;
import java.util.*;

class CGI {
    public int _cgiptr;
    
    static { 
	try {
	    System.loadLibrary("neo_cgi");
	} catch ( UnsatisfiedLinkError e ) {
	    System.out.println("Could not load neo_cgi.so");
	    System.exit(1);
	}
    }
    
    public CGI() {
	_cgiptr = _init();
    }

    private static native int _init();
    private static native void parse();
};
