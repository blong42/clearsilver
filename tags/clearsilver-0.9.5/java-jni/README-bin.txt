---------------------------------
Clearsilver JavaJNI wrapper
by David Jeske
Distributed under the Neotonic ClearSilver License
----------------------------------

This is a binary compile of a Java JNI wrapper for the ClearSilver
templating library. This version is compiled on:

  x86 Linux (Linux 2.2.x - RedHat 6.2)

INSTALLING

You must put the native library (libclearsilver-jni.so) into a standard 
library location (i.e. like /lib), or make sure that Java can 
find it by using the java command line directive: 

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
-
