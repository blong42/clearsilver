
import java.io.*;
import java.util.*;

class CSTest {

    public static void main( String [] args ) throws IOException {
	org.clearsilver.HDF hdf = new org.clearsilver.HDF();

        System.out.println("Testing HDF set and dump\n");
	hdf.setValue("Foo.Bar","10");
        hdf.setValue("Foo.Baz","20");
        System.out.println( hdf.dump() );

	String foo = hdf.getValue("Foo.Bar", "");
        System.out.println("Testing HDF get\n");
	System.out.println( foo );

	System.out.println( "----" );

	org.clearsilver.CS cs = new org.clearsilver.CS(hdf);
	
        System.out.println("Testing HDF parse/render\n");
	String tmplstr = "Foo.Bar:<?cs var:Foo.Bar ?>\nFoo.Baz:<?cs var:Foo.Baz ?>\n";
	System.out.println(tmplstr);
	System.out.println("----");

	cs.parseStr(tmplstr);
	System.out.println(cs.render());

        // test registered functions
        System.out.println("Testing registered string functions\n");
        hdf.setValue("Foo.EscapeTest","abc& 231<>/?");
   
        tmplstr = " <?cs var:url_escape(Foo.EscapeTest) ?> <?cs var:html_escape(Foo.EscapeTest) ?>";

	cs.parseStr(tmplstr);
	System.out.println(cs.render());

	cs = new org.clearsilver.CS(hdf);

        System.out.println("Testing white space stripping\n");
	// test white space stripping
	tmplstr = "      <?cs var:Foo.Bar ?> This is a       string     without whitespace stripped";
	cs.parseStr(tmplstr);
	System.out.println(cs.render());

        hdf.setValue("ClearSilver.WhiteSpaceStrip", "1");
	System.out.println(cs.render());

	// Now, test debug dump
        System.out.println("Testing debug dump\n");
        hdf.setValue("ClearSilver.DisplayDebug", "1");
	System.out.println(cs.render());
	
        System.out.println("Final HDF dump\n");
        System.out.println( hdf.dump() );
    }
};
