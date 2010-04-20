package org.clearsilver.test;

import org.clearsilver.CS;
import org.clearsilver.CSFileLoader;
import org.clearsilver.CSUtil;
import org.clearsilver.ClearsilverFactory;
import org.clearsilver.FactoryLoader;
import org.clearsilver.HDF;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;


public class CSTest {

  private static final ClearsilverFactory CLEARSILVER_FACTORY =
    FactoryLoader.getClearsilverFactory();

  static class CSTestLoader implements CSFileLoader {
    public String load(HDF hdf, String filename) throws IOException {
      String loadPath = hdf.getValue(CSUtil.HDF_LOADPATHS + ".0", "");
      System.out.println("CSTestLoader::Load " + filename + "\n");
      File f = new File(loadPath, filename);
      String data = readFile(f);
      System.out.println("---- file begin ----\n" + data +
          "\n---- file end ----\n");
      return data;
    }

    private String readFile(File file) throws IOException {
      InputStreamReader fin = new InputStreamReader(new FileInputStream(file),
          "UTF-8");
      StringBuilder sb = new StringBuilder(1024);
      char[] charbuf = new char[1024];
      int len = 0;
      while ((len = fin.read(charbuf)) != -1) {
        sb.append(charbuf, 0, len);
      }
      return sb.toString();
    }
  };

  static class TestClearsilverFactory implements ClearsilverFactory {
    private final ClearsilverFactory clearsilverFactory;
    private final String loadPath;

    TestClearsilverFactory(ClearsilverFactory clearsilverFactory,
                           String loadPath) {
      this.clearsilverFactory = clearsilverFactory;
      this.loadPath = loadPath;
    }

    public CS newCs(HDF hdf) {
      return clearsilverFactory.newCs(hdf);
    }

    public CS newCs(HDF hdf, HDF globalHdf) {
      return clearsilverFactory.newCs(hdf, globalHdf);
    }

    public HDF newHdf() {
      HDF hdf = clearsilverFactory.newHdf();
      hdf.setValue(CSUtil.HDF_LOADPATHS + ".0", loadPath);
      return hdf;
    }
  }

  public static void main( String [] args ) throws IOException {

    if (args.length == 0) {
      System.err.println("Must specify testdata directory as argument.");
      System.exit(1);
    }

    String testDataDir = args[0];

    System.out.println("Using testdata directory: " + testDataDir);

    ClearsilverFactory clearsilverFactory =
        new TestClearsilverFactory(CLEARSILVER_FACTORY, testDataDir);

    org.clearsilver.HDF hdf = clearsilverFactory.newHdf();

    System.out.println("Testing HDF set and dump\n");
    hdf.setValue("Foo.Bar","10");
    hdf.setValue("Foo.Baz","20");
    System.out.println( hdf.dump() );

    System.out.println("Testing HDF get\n");
    String foo = hdf.getValue("Foo.Bar", "30");
    System.out.println( foo );
    foo = hdf.getValue("Foo.Baz", "30");
    System.out.println( foo );

    System.out.println( "----" );

    System.out.println("Testing HDF setSymLink\n");
    hdf.setSymLink("Foo.Baz2","Foo.Baz");
    foo = hdf.getValue("Foo.Baz", "30");
    System.out.println( foo );

    System.out.println( "----" );

    System.out.println("Testing HDF get where default value is null\n");
    foo = hdf.getValue("Foo.Bar", null);
    System.out.println("foo = " + foo);
    foo = hdf.getValue("Foo.Nonexistent", null);
    System.out.println("foo = " + foo);

    System.out.println( "----" );

    int fooInt = hdf.getIntValue("Foo.Bar", 30);
    System.out.println("Testing HDF get int\n");
    System.out.println( fooInt );

    System.out.println( "----" );

    org.clearsilver.CS cs = clearsilverFactory.newCs(hdf);

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

    cs = clearsilverFactory.newCs(hdf);

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

        // Now, test reading an HDF file from disk
    System.out.println("Testing HDF.readFile()\n");
    HDF file_hdf = clearsilverFactory.newHdf();
    file_hdf.readFile("testdata/test1.hdf");
    System.out.println(file_hdf.dump());

    System.out.println("Testing HDF.readFile() for a file that doesn't exist");
    try {
      file_hdf.readFile("testdata/doesnt_exist.hdf");
    } catch (Exception e) {
      // The error message contains line numbers for functions in
      // neo_hdf.c, and I don't want this test to fail if the line numbers
      // change, so I'm not going to print out the exception message here.
      // The important thing to test here is that an exception is thrown
      // System.out.println(e + "\n");
      System.out.println("Caught exception of type " + e.getClass().getName() + "\n");
    }

    System.out.println("Testing HDF.writeFile()\n");
    file_hdf.writeFile("test1_out.hdf");

    System.out.println("Testing HDF.writeString()\n");
    System.out.println(file_hdf.writeString());

    System.out.println("Testing HDF.getObj()");
    HDF foo_hdf = file_hdf.getObj("Foo");
    System.out.println(foo_hdf.dump());

    System.out.println("Testing HDF.objName()");
    System.out.println("Should be \"Foo\": " + foo_hdf.objName());
    System.out.println("Should be \"Bar\": "
                       + foo_hdf.getObj("Bar").objName());
    System.out.println("Should be null: " + file_hdf.objName() + "\n");

    System.out.println("Testing HDF.objValue()");
    System.out.println("Value of Foo.Bar: "
                       + foo_hdf.getObj("Bar").objValue());
    System.out.println("Value of root node: " + file_hdf.objValue() + "\n");

    System.out.println("Testing HDF.objChild()");
    HDF child_hdf = foo_hdf.objChild();
    System.out.println("First child name: " + child_hdf.objName() + "\n");

    System.out.println("Testing HDF.objNext()");
    HDF next_hdf = child_hdf.objNext();
    System.out.println("Next child name: " + next_hdf.objName());
    next_hdf = next_hdf.objNext();
    System.out.println("Next child (should be null): " + next_hdf + "\n");

    System.out.println("Testing HDF.copy()");
    HDF one = clearsilverFactory.newHdf();
    one.setValue("name", "barneyb");
    one.setValue("age", "25");
    HDF two = clearsilverFactory.newHdf();
    two.setValue("entity.type", "person");
    two.copy("entity.value", one);
    System.out.println("name should be barneyb: " +
        two.getValue("entity.value.name", "--undefined--") +"\n");

    System.out.println("Testing HDF.exportDate()");
    HDF date_hdf = clearsilverFactory.newHdf();
    date_hdf.exportDate("DatePST", "US/Pacific", 1142308218);
    date_hdf.exportDate("DateEST", "US/Eastern", 1142308218);
    System.out.println(date_hdf.dump());

    // Test default escaping mode: html
    HDF escape_hdf = clearsilverFactory.newHdf();
    System.out.println("Testing escape mode: html");
    System.out.println("Config.VarEscapeMode = \"html\"");
    escape_hdf.setValue("Config.VarEscapeMode", "html");
    cs = clearsilverFactory.newCs(escape_hdf);

    System.out.println("Some.HTML = " +
        "<script src=\"some.js\">alert('123');</script>");
    escape_hdf.setValue("Some.HTML",
        "<script src=\"some.js\">alert('123');</script>");
    tmplstr = "Default HTML escaping: <?cs var:Some.HTML ?>\n";
    System.out.println(tmplstr);
    System.out.println("----");
    cs.parseStr(tmplstr);
    System.out.println(cs.render());

    // Test default escaping mode: js
    escape_hdf = clearsilverFactory.newHdf();
    System.out.println("Testing escape mode: js");
    System.out.println("Config.VarEscapeMode = \"js\"");
    escape_hdf.setValue("Config.VarEscapeMode", "js");
    cs = clearsilverFactory.newCs(escape_hdf);

    System.out.println("Some.HTML = " +
        "<script src=\"some.js\">alert('123');</script>");
    escape_hdf.setValue("Some.HTML",
        "<script src=\"some.js\">alert('123');</script>");
    tmplstr = "Default JS escaping: <?cs var:Some.HTML ?>\n";
    System.out.println(tmplstr);
    System.out.println("----");
    cs.parseStr(tmplstr);
    System.out.println(cs.render());

    // Test default escaping mode: url
    escape_hdf = clearsilverFactory.newHdf();
    System.out.println("Testing escape mode: url");
    System.out.println("Config.VarEscapeMode = \"url\"");
    escape_hdf.setValue("Config.VarEscapeMode", "url");
    cs = clearsilverFactory.newCs(escape_hdf);

    System.out.println("Some.HTML = " +
        "<script src=\"some.js\">alert('123');</script>");
    escape_hdf.setValue("Some.HTML",
        "<script src=\"some.js\">alert('123');</script>");
    tmplstr = "Default URL escaping: <?cs var:Some.HTML ?>\n";
    System.out.println(tmplstr);
    System.out.println("----");
    cs.parseStr(tmplstr);
    System.out.println(cs.render());

    // Test escape blocks
    escape_hdf = clearsilverFactory.newHdf();
    System.out.println("Testing escape blocks: none");
    System.out.println("Config.VarEscapeMode = \"none\"");
    escape_hdf.setValue("Config.VarEscapeMode", "none");
    cs = clearsilverFactory.newCs(escape_hdf);

    System.out.println("Some.HTML = " +
        "<script src=\"some.js\">alert('123');</script>");
    escape_hdf.setValue("Some.HTML",
        "<script src=\"some.js\">alert('123');</script>");
    tmplstr = "url escape block: \n" +
      "<?cs escape: \"url\"?>" +
      "  <?cs var:Some.HTML ?>" +
      "<?cs /escape ?>\n" +
      "js escape block: \n" +
      "<?cs escape: \"js\"?>" +
      "  <?cs var:Some.HTML ?>" +
      "<?cs /escape ?>\n" +
      "html escape block: \n" +
      "<?cs escape: \"html\"?>" +
      "  <?cs var:Some.HTML ?>" +
      "<?cs /escape ?>\n";
    System.out.println(tmplstr);
    System.out.println("----");
    cs.parseStr(tmplstr);
    System.out.println(cs.render());

    System.out.println("Testing HDF.readFile() with callback\n");
    file_hdf = clearsilverFactory.newHdf();
    file_hdf.setFileLoader(new CSTestLoader());
    file_hdf.readFile("testdata/test1.hdf");
    System.out.println(file_hdf.dump());

    System.out.println("Testing CS.parseFile() with callback\n");
    cs = clearsilverFactory.newCs(file_hdf);
    cs.setFileLoader(new CSTestLoader());
    cs.parseFile("testdata/test.cs");
    System.out.println(cs.render());

    System.out.println("Testing CS.parseFile() with parse time callback\n");
    cs = clearsilverFactory.newCs(file_hdf);
    cs.setFileLoader(new CSTestLoader());
    cs.parseFile("testdata/test2a.cs");
    System.out.println(cs.render());

    System.out.println("Testing CS.parseFile() with render time callback\n");
    cs = clearsilverFactory.newCs(file_hdf);
    cs.setFileLoader(new CSTestLoader());
    cs.parseFile("testdata/test2b.cs");
    System.out.println(cs.render());
  }
};
