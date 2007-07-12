
using System;
using System.Runtime.InteropServices;

namespace Clearsilver {

// opaque types
public unsafe struct HDF {};
public unsafe struct NEOERR {};

public unsafe class Hdf {

  [DllImport("libneo", EntryPoint="hdf_init")]
  private static extern unsafe NEOERR* hdf_init(HDF **foo);

  // NEOERR* hdf_set_value (HDF *hdf, char *name, char *value)
  [DllImport("libneo")]
  private static unsafe extern NEOERR* hdf_set_value(HDF *hdf,
       [MarshalAs(UnmanagedType.LPStr)] 
        string name,
       [MarshalAs(UnmanagedType.LPStr)] 
        string value);

  // char* hdf_get_value (HDF *hdf, char *name, char *defval)

  [DllImport("libneo")]
  [return: MarshalAs(UnmanagedType.LPStr)] 
  private static unsafe extern string hdf_get_value(HDF *hdf,
       [MarshalAs(UnmanagedType.LPStr)] 
        string name,
       [MarshalAs(UnmanagedType.LPStr)] 
        string defval);

  // NEOERR* hdf_dump (HDF *hdf, char *prefix);

  [DllImport("libneo", EntryPoint="hdf_dump")]
  private static extern void hdf_dump(
       HDF *hdf,
       [MarshalAs(UnmanagedType.LPStr)]
         string prefix);

  // HDF* hdf_get_obj (HDF *hdf, char *name)

  [DllImport("libneo", EntryPoint="hdf_get_obj")]
  private static extern HDF* hdf_get_obj(
     HDF *hdf, 
       [MarshalAs(UnmanagedType.LPStr)]
     string name);


  // -----------------------------------------------------------

    public HDF *hdf_root;

    public Hdf() {
      fixed (HDF **hdf_ptr = &hdf_root) {
	hdf_init(hdf_ptr);
      }
      // Console.WriteLine((int)hdf_root);
    }

    public void setValue(string name,string value) {
         hdf_set_value(hdf_root,name,value);
         
    }
    public string getValue(string name,string defvalue) {
         return hdf_get_value(hdf_root,name,defvalue);
    }

    public void test() {
       hdf_set_value(hdf_root,"b","1");
       // hdf_read_file(hdf_root,"test.hdf");
       // Console.WriteLine("b ", hdf_get_value(hdf_root,"b","5"));
       hdf_dump(hdf_root,null);

       // HDF *n = hdf_get_obj(hdf_root,"b");
       // Console.WriteLine("object name {0}", 
       // Marshal.PtrToStringAnsi((IntPtr)n->name));
    }

};

unsafe struct CSPARSE {};

public class CSTContext {
   unsafe CSPARSE *csp;
   unsafe public CSTContext(Hdf hdf) {
     fixed (CSPARSE **csp_ptr = &csp) {
       cs_init(csp_ptr, hdf.hdf_root);
     }
   } 

   [DllImport("libneo")]
   extern static unsafe NEOERR *cs_init (CSPARSE **parse, HDF *hdf);

   public unsafe void parseFile(string filename) {
      cs_parse_file(csp,filename);
   }

   [DllImport("libneo")]
   extern static unsafe NEOERR *cs_parse_file (CSPARSE *parse, 
       [MarshalAs(UnmanagedType.LPStr)] 
       string path);

//   [DllImport("libneo")]
//   extern static unsafe NEOERR *cs_parse_string (CSPARSE *parse, 
//                    char *buf, 
//                    size_t blen);


   //  NEOERR *cs_render (CSPARSE *parse, void *ctx, CSOUTFUNC cb);
   //  typedef NEOERR* (*CSOUTFUNC)(void *ctx, char *more_str_bytes);

   [DllImport("libneo")]
   extern static unsafe NEOERR *cs_render (CSPARSE *parse, 
           void *ctx, 
           [MarshalAs(UnmanagedType.FunctionPtr)]
           CSOUTFUNC cb);

   private unsafe delegate NEOERR* CSOUTFUNC(void* ctx, sbyte* more_bytes);

   private class OutputBuilder {
      private string output = "";
      public unsafe NEOERR* handleOutput(void* ctx, sbyte* more_bytes) {
           // add the more_bytes to the current string buffer

           output += new String(more_bytes);
           // Console.WriteLine("handleOutput called");
           return null;
      }
      public string result() {
         return output;
      }
   }

   public unsafe string render() {
     OutputBuilder ob = new OutputBuilder();
     cs_render(csp,null,new CSOUTFUNC(ob.handleOutput));
     return ob.result();
   }


   [DllImport("libneo")]
   extern static unsafe void cs_destroy (CSPARSE **parse);

};


} // namespace Clearsilver
