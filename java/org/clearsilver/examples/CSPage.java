package org.clearsilver.examples;

import org.clearsilver.CS;
import org.clearsilver.HDF;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Enumeration;

import javax.servlet.ServletException;
import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/**
 * The simplest possible servlet.
 *
 */

public class CSPage extends HttpServlet {
    public HDF hdf;
    public CS cs;
    public boolean page_debug = true; 

    public void doGet(HttpServletRequest request,
                      HttpServletResponse response)
        throws IOException, ServletException
    {


        PrintWriter out = response.getWriter();
	hdf = new HDF();
	cs = new CS(hdf);

	// HTTP headers
        Enumeration e = request.getHeaderNames();
        while (e.hasMoreElements()) {
            String headerName = (String)e.nextElement();
            String headerValue = request.getHeader(headerName);
	    hdf.setValue("HTTP." + headerName,headerValue);
        }


	hdf.setValue("HTTP.PATH_INFO",request.getPathInfo());
	hdf.setValue("CGI.QueryString",request.getQueryString());
	hdf.setValue("CGI.RequestMethod",request.getMethod());

	// Querystring paramaters
	e = request.getParameterNames();
	while (e.hasMoreElements()) {
	    String paramName = (String)e.nextElement();
	    String paramValue = request.getParameter(paramName);
	    hdf.setValue("Query." + paramName,paramValue);
	}


	// Cookies

        Cookie[] cookies = request.getCookies();
        if (cookies.length > 0) {
            for (int i = 0; i < cookies.length; i++) {
                Cookie cookie = cookies[i];
		hdf.setValue("Cookie." + cookie.getName(),cookie.getValue());
            }
        }

	// CGI example
	// check for Actions
	

	// then call display method
	this.display();

	// run required page template through CS
	// cs.parseFile(a_template_file);

	// Page Output


	/* first do cookies 

        String cookieName = request.getParameter("cookiename");
        String cookieValue = request.getParameter("cookievalue");
        if (cookieName != null && cookieValue != null) {
            Cookie cookie = new Cookie(cookieName, cookieValue);
            response.addCookie(cookie);
            out.println("<P>");
            out.println(rb.getString("cookies.set") + "<br>");
            out.print(rb.getString("cookies.name") + "  " + cookieName +
		      "<br>");
            out.print(rb.getString("cookies.value") + "  " + cookieValue);
        }

	*/

        response.setContentType("text/html");
	out.print(cs.render());


	// debug
	if (page_debug) {
	  out.print("<HR><PRE>");
	  out.print(hdf.dump());
	  out.print("</PRE>");
	}

    }

    public void doPost(HttpServletRequest request,
                      HttpServletResponse response) throws IOException, ServletException {
        doGet(request, response);
    }

    public void display() {
	hdf.setValue("Foo.Bar","1");
	cs.parseStr("Hello Clearsilver<p><TABLE BORDER=1><TR><TD>Foo.Bar</TD><TD><?cs var:Foo.Bar ?></TD></TR></TABLE>");
    }


}



