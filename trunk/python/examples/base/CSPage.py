#!/neo/opt/bin/python

import neo_cgi
import sys, os, string
import time
from log import *

# errors thrown...
NoPageName = "NoPageName"
NoDisplayMethod = "NoDisplayMethod"

# errors signaled back to here
Redirected = "Redirected"
DisplayDone = "DisplayDone"
DisplayError = "DisplayError"

class Context:
    def __init__ (self):
        self.argv = sys.argv
        self.stdin = sys.stdin
        self.stdout = sys.stdout
        self.stderr = sys.stderr
        self.environ = os.environ

class CSPage:
    def __init__(self, context, pagename=0,readDefaultHDF=1,israwpage=0,**parms):
        if pagename == 0:
            raise NoPageName, "missing pagename"
        self.pagename = pagename
        self.readDefaultHDF = readDefaultHDF
        self._israwpage = israwpage
        self.context = context
        self._pageparms = parms

        self._error_template = None

        self.page_start_time = time.time()
        neo_cgi.cgiWrap(context.stdin, context.stdout, context.environ)
        neo_cgi.IgnoreEmptyFormVars(1)
        self.ncgi = neo_cgi.CGI()
        self.ncgi.parse()
        self._path_num = 0
        domain = self.ncgi.hdf.getValue("CGI.ServerName","")
        domain = self.ncgi.hdf.getValue("HTTP.Host", domain)
        self.domain = domain
        self.subclassinit()
        self.setPaths([self.ncgi.hdf.getValue("CGI.DocumentRoot","")])

    def subclassinit(self):
        pass

    def setPaths(self, paths):
        for path in paths:  
            self.ncgi.hdf.setValue("hdf.loadpaths.%d" % self._path_num, path)
            self._path_num = self._path_num + 1
            
    def redirectUri(self,redirectTo):
        ncgi = self.ncgi
        if ncgi.hdf.getIntValue("Cookie.debug",0) == 1:
            ncgi.hdf.setValue("CGI.REDIRECT_TO",redirectTo)
            ncgi.display("dbg/redirect.cs")
            print "<PRE>"
            print neo_cgi.htmlEscape(ncgi.hdf.dump())
            print "</PRE>"
            raise DisplayDone

        self.ncgi.redirectUri(redirectTo)
        raise Redirected, "redirected To: %s" % redirectTo

    ## ----------------------------------
    ## methods to be overridden in subclass when necessary:

    def setup(self):
        pass

    def display(self):
        raise NoDisplayMethod, "no display method present in %s" % repr(self)

    def main(self):
        self.setup()
        self.handle_actions()
        self.display()

    ## ----------------------------------
        
    def handle_actions(self):
        hdf = self.ncgi.hdf
        hdfobj = hdf.getObj("Query.Action")
        if hdfobj:
            firstchild = hdfobj.child()
            if firstchild:
                action = firstchild.name()
                if firstchild.next():
                    raise "multiple actions present!!!"

                method_name = "Action_%s" % action
                method = getattr(self,method_name)
                apply(method,[])

    def start(self):
        SHOULD_DISPLAY = 1
        if self._israwpage:
            SHOULD_DISPLAY = 0
        
        ncgi = self.ncgi
        
        if self.readDefaultHDF:
            try:
                if not self.pagename is None:
                    ncgi.hdf.readFile("%s.hdf" % self.pagename)
            except:
                log("Error reading HDF file: %s.hdf" % (self.pagename))

        DISPLAY_ERROR = 0
        ERROR_MESSAGE = ""
        # call page main function!
        try:
            self.main()
        except DisplayDone:
            SHOULD_DISPLAY = 0
        except Redirected:
            # catch redirect exceptions
            SHOULD_DISPLAY = 0
        except DisplayError, num:
            ncgi.hdf.setValue("Query.error", str(num))
            if self._error_template:
                ncgi.hdf.setValue("Content", self._error_template)
            else:
                DISPLAY_ERROR = 1
        except:
            SHOULD_DISPLAY = 0
            DISPLAY_ERROR = 1
            
            import handle_error
            handle_error.handleException("Display Failed!")
            ERROR_MESSAGE = handle_error.exceptionString()

        if DISPLAY_ERROR:
            print "Content-Type: text/html\n\n"

            # print the page

            print "<H1> Error in Page </H1>"
            print "A copy of this error report has been submitted to the developers. "
            print "The details of the error report are below."


            print "<PRE>"
            print neo_cgi.htmlEscape(ERROR_MESSAGE)
            print "</PRE>\n"
            # print debug info always on page error...
            print "<HR>\n"
            print "<PRE>"
            print neo_cgi.htmlEscape(ncgi.hdf.dump())
            print "</PRE>"


        etime = time.time() - self.page_start_time
        ncgi.hdf.setValue("CGI.debug.execute_time","%f" % (etime))

        if SHOULD_DISPLAY and self.pagename:
            debug_output = ncgi.hdf.getIntValue("page.debug",ncgi.hdf.getIntValue("Cookie.debug",0))

            # hijack the built-in debug output method...
            if ncgi.hdf.getValue("Query.debug","") == ncgi.hdf.getValue("Config.DebugPassword","1"):
                ncgi.hdf.setValue("Config.DebugPassword","CSPage.py DEBUG hijack (%s)" %
                    ncgi.hdf.getValue("Config.DebugPassword",""))
                debug_output = 1

            if not debug_output:
              ncgi.hdf.setValue("Config.CompressionEnabled","1")

            # default display
            template_name = ncgi.hdf.getValue("Content","%s.cs" % self.pagename)
            # ncgi.hdf.setValue ("cgiout.charset", "utf-8");

            try:
                ncgi.display(template_name)
            except:
                print "Content-Type: text/html\n\n"
                print "CSPage: Error occured"
                import handle_error
                print "<pre>" + handle_error.exceptionString() + "</pre>"
                debug_output = 1
                 

            # debug output
            if debug_output:
                print "<HR>\n"
                print "CSPage Debug, Execution Time: %5.3f<BR><HR>" % (etime)
                print "<PRE>"
                print neo_cgi.htmlEscape(ncgi.hdf.dump())
                print "</PRE>"
                # ncgi.hdf.setValue("hdf.DEBUG",ncgi.hdf.dump())
                # ncgi.display("debug.cs")
                
        script_name = ncgi.hdf.getValue("CGI.ScriptName","")
        if script_name:
            script_name = string.split(script_name,"/")[-1]
            
        log ("[%s] etime/dtime: %5.3f/%5.3f %s (%s)" % (self.domain, etime, time.time() - etime - self.page_start_time,  script_name, self.pagename))

    # a protected output function to catch the output errors that occur when
    # the server is either restarted or the user pushes the stop button on the
    # browser
    def output(self, str):
        try:
            self.context.stdout.write(str)
        except IOError, reason:
            log("IOError: %s" % (repr(reason)))
            raise DisplayDone


    def allQuery (self, s):
        l = []
        if self.ncgi.hdf.getValue ("Query.%s.0" % s, ""):
          obj = self.ncgi.hdf.getChild ("Query.%s" % s)
          while obj:
            l.append(obj.value())
            obj = obj.next()
        else:
          t = self.ncgi.hdf.getValue ("Query.%s" % s, "")
          if t: l.append(t)
        return l
