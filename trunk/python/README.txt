------------------------
Python Clearsilver Module
By Brandon Long

Windows port by David Jeske
------------------------

This is the python clearsilver module. You can see the online
documentation for the Python API here:

  http://www.clearsilver.net/docs/python/

COMPILING on UNIX

Compiling from source on Linux (UNIX) should be pretty
straightforward. Simply make sure that the Makefile and top level
rules.mk point to your Python installation of choice. Comments or
questions to blong@fiction.net

WINDOWS BINARY

If you are using Python2.2, we highly recommend you download the
pre-built binary. You can download a windows binary of the module from
the clearsilver download page here:

  http://www.clearsilver.net/downloads/

COMPILING on Windows

If you must compile on windows, here is the set of steps you need to
follow for Python22. Comments or questions to jeske@chat.net.

1) Download and Install Python2.2 from:

   http://www.python.org/ftp/python/2.2.1/Python-2.2.1.exe

2) Download and Install Mingw32-2.0 and binutils from:

   http://prdownloads.sourceforge.net/mingw/MinGW-2.0.0-3.exe?download
   http://prdownloads.sourceforge.net/mingw/binutils-2.13-20020903-1.tar.gz?download

3) Download and Install UNIX Tools and Make from:

   http://unxutils.sourceforge.net/UnxUtils.zip

   http://prdownloads.sourceforge.net/mingw/make-3.79.1-20010722.tar.gz 

4) Add the mingw32 binary directory to your path

5) Build the libpython22.a import library using these steps:

   a) Download the Py-mingw32 tools:

     http://starship.python.net/crew/kernr/mingw32/Py-mingw32-tools.zip

   b) extract them to C:\Python22\libs

   c) edit lib2def.py and replace python15->python22, Python15->Python22

   d) execute these commands:

       C:\Python22\libs> lib2def.py python22.lib > python22.def
       C:\Python22\libs> dlltool --dllname python22.dll --def python22.def --output-lib libpython22.a

   Reference:

   http://starship.python.net/pipermail/mmtk/2002/000398.html

6) Check the library and include paths in the neotonic/Python/Makefile

7) Then type "make" in the neotonic\python directory... 
