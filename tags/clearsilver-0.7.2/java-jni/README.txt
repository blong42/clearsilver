----------------------------------
Clearsilver JavaJNI wrapper
by David Jeske
Distributed under the Neotonic ClearSilver License
----------------------------------

This is a Java Native Interface (JNI) wrapper for the ClearSilver
templating library. The files in this directory build into both
a clearsilver.jar file with Java classes, and a native library
libclearsilver-jni.so. 

BUILDING

After building clearsilver, just type "make" twice. The first
time you build it builds the depend, and throw some random error,
I don't know why.

INSTALLING

You must put the native library into a standard library location 
(i.e. like /lib), or make sure that Java can find it by using 
the java command line directive: 

  -Djava.library.path=/somewhere/else

Then you must put the clearsilver.jar file into your java 
CLASSPATH.

USING

See the example CSTest.java for an example of how to import
and use the clearsilver objects. 

USING in a servlet

Since the most common usage of Clearsilver is in a servlet, 
we've provided some examples of how to use it in the servlet/
directory.


