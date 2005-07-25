/* $Id: HelloCSServlet.java,v 1.1 2002-09-20 22:13:08 jeske Exp $
 *
 */

import java.io.*;
import java.text.*;
import java.util.*;
import javax.servlet.*;
import javax.servlet.http.*;

import org.clearsilver.*;

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



