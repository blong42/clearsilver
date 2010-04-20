package org.clearsilver.examples;

import org.clearsilver.CS;
import org.clearsilver.HDF;

import java.io.IOException;
import java.io.PrintWriter;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/**
 * The simplest possible Clearsilver servlet.
 *
 */

public class HelloCSServlet extends HttpServlet {

    public void doGet(HttpServletRequest request,
                      HttpServletResponse response)
        throws IOException, ServletException
    {


        PrintWriter out = response.getWriter();
	HDF hdf = new HDF();

	hdf.setValue("Foo.Bar","1");

	CS cs = new CS(hdf);
	cs.parseStr("Hello Clearsilver<p>Foo.Bar: <?cs var:Foo.Bar ?>");



        response.setContentType("text/html");
	out.print(cs.render());
    }
}



