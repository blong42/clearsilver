using System;
using System.Runtime.InteropServices;

using Clearsilver;


public class CSPerfTest {
   public delegate void test_function(int INDEX);

    public static void timefunk(String label, test_function f, int count) {
      int start, end, elapsed;
      start = Environment.TickCount;
      f(count);
      end = Environment.TickCount;

      elapsed = end-start;

      Console.WriteLine(label + "   " + count + " elapsed: " + (elapsed / 1000.0));
    }

   public static unsafe int Main(string[] argv) {
      Console.WriteLine("C# Clearsilver wrapper performance test");
      Hdf h = new Hdf();

      h.setValue("foo.1","1");
      h.setValue("foo.2","2");

      int call_count = 100000;
      int start = Environment.TickCount;
      for (int i=0;i<call_count;i++) {
         h.setValue(String.Format("foo.{0}",i),"5");
      }
      int end = Environment.TickCount;

      Console.WriteLine("call count = {0}, time = {1} ms - time per call {2} ns", 
           call_count, end-start, (((float)end-start)/call_count) * 1000);
      

      CSTContext cs = new CSTContext(h);
//      cs.parseFile("test.cst");
      Console.WriteLine(cs.render());
      
      return 0;
   }
  


}

